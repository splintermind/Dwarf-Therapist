/*
Dwarf Therapist
Copyright (c) 2009 Trey Stout (chmod)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
#include <QInputDialog>
#include <QMessageBox>
#include <QtDebug>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/ptrace.h>
#include <sys/uio.h>
#include <sys/user.h>
#include <sys/mman.h>
#include <errno.h>
#include <error.h>
#include <wait.h>

#include "dfinstance.h"
#include "dfinstancelinux.h"
#include "defines.h"
#include "dwarf.h"
#include "utils.h"
#include "gamedatareader.h"
#include "memorylayout.h"
#include "memorysegment.h"
#include "truncatingfilelogger.h"

#include "cp437codec.h"

struct STLStringHeader {
    quint32 length;
    quint32 capacity;
    qint32 refcnt;
};

DFInstanceLinux::DFInstanceLinux(QObject* parent)
    : DFInstance(parent)
    , m_executable(NULL)
    , m_inject_addr(-1)
    , m_alloc_start(0)
    , m_alloc_end(0)
{
}

DFInstanceLinux::~DFInstanceLinux() {
    if (m_attach_count > 0) {
        detach();
    }
}

QVector<uint> DFInstanceLinux::enumerate_vector(const uint &addr) {
    QVector<uint> addrs;
    if (!addr)
        return addrs;

    attach();
    VIRTADDR tmp_addr = 0;
    VIRTADDR start = read_addr(addr);
    VIRTADDR end = read_addr(addr + 4);
    int bytes = end - start;
    if(check_vector(start,end,addr)){
        QByteArray data(bytes, 0);
        int bytes_read = read_raw(start, bytes, data);
        if (bytes_read != bytes && m_layout->is_complete()) {
            LOGW << "Tried to read" << bytes << "bytes but only got" << bytes_read;
            detach();
            return addrs;
        }
        for(int i = 0; i < bytes; i += 4) {
            tmp_addr = decode_dword(data.mid(i, 4));
            addrs << tmp_addr;
        }
    }
    detach();
    return addrs;
}

uint DFInstanceLinux::calculate_checksum() {
    // ELF binaries don't seem to store a linker timestamp, so just MD5 the file.
    uint md5 = 0; // we're going to throw away a lot of this checksum we just need 4bytes worth
    QProcess *proc = new QProcess(this);
    QStringList args;
    args << "md5sum";
    args << QString("/proc/%1/exe").arg(m_pid);
    proc->start("/usr/bin/env", args);
    if (proc->waitForReadyRead(3000)) {
        QByteArray out = proc->readAll();
        QString str_md5(out);
        QStringList chunks = str_md5.split(" ");
        str_md5 = chunks[0];
        bool ok;
        md5 = str_md5.mid(0, 8).toUInt(&ok,16); // use the first 8 bytes
        TRACE << "GOT MD5:" << md5;
    }
    return md5;
}


QString DFInstanceLinux::read_string(const VIRTADDR &addr) {
    VIRTADDR buffer_addr = read_addr(addr);
    int upper_size = 256;
    QByteArray buf(upper_size, 0);
    read_raw(buffer_addr, upper_size, buf);

    buf.truncate(buf.indexOf(QChar('\0')));
    CP437Codec *c = new CP437Codec();
    return c->toUnicode(buf);
}

int DFInstanceLinux::write_string(const VIRTADDR &addr, const QString &str) {
    // Ensure this operation is done as one transaction
    attach();
    VIRTADDR buffer_addr = get_string(str);
    if (buffer_addr)
        // This unavoidably leaks the old buffer; our own
        // cannot be deallocated anyway.
        write_raw(addr, sizeof(VIRTADDR), &buffer_addr);
    detach();
    return buffer_addr ? str.length() : 0;
}

int DFInstanceLinux::write_int(const VIRTADDR &addr, const int &val) {
    return write_raw(addr, sizeof(int), (void*)&val);
}

int DFInstanceLinux::wait_for_stopped() {
    int status;
    while(true) {
        TRACE << "waiting for proc to stop";
        pid_t w = waitpid(m_pid, &status, 0);
        if (w == -1) {
            // child died
            perror("wait inside attach()");
            LOGE << "child died?";
            exit(-1);
        }
        if (WIFSTOPPED(status)) {
            break;
        }
        TRACE << "waitpid returned but child wasn't stopped, keep waiting...";
    }
    return status;
}

bool DFInstanceLinux::attach() {
    TRACE << "STARTING ATTACH" << m_attach_count;
    if (is_attached()) {
        m_attach_count++;
        TRACE << "ALREADY ATTACHED SKIPPING..." << m_attach_count;
        return true;
    }

    if (ptrace(PTRACE_ATTACH, m_pid, 0, 0) == -1) { // unable to attach
        perror("ptrace attach");
        LOGE << "Could not attach to PID" << m_pid;
        return false;
    }

    wait_for_stopped();

    m_attach_count++;
    TRACE << "FINISHED ATTACH" << m_attach_count;
    return m_attach_count > 0;
}

bool DFInstanceLinux::detach() {
    //TRACE << "STARTING DETACH" << m_attach_count;
    if(m_memory_file.isOpen())
        m_memory_file.close();
    m_attach_count--;
    if (m_attach_count > 0) {
        TRACE << "NO NEED TO DETACH SKIPPING..." << m_attach_count;
        return true;
    }

    ptrace(PTRACE_DETACH, m_pid, 0, 0);
    TRACE << "FINISHED DETACH" << m_attach_count;
    return m_attach_count > 0;
}

int DFInstanceLinux::read_raw_ptrace(const VIRTADDR &addr, size_t bytes, QByteArray &buffer) {
    // try to attach, will be ignored if we're already attached
    attach();

    // open the memory virtual file for this proc (can only read once
    // attached and child is stopped    
    if (!m_memory_file.isOpen() && !m_memory_file.open(QIODevice::ReadOnly)) {
        LOGE << "Unable to open" << m_memory_file.fileName();
        detach();
        return 0;
    }

    int bytes_read = 0; // tracks how much we've read of what was asked for
    int step_size = 0x1000; // how many bytes to read each step
    QByteArray chunk(step_size, 0); // our temporary memory container
    buffer.fill(0, bytes); // zero our buffer
    int steps = bytes / step_size;
    if (bytes % step_size)
        steps++;

    for(VIRTADDR ptr = addr; ptr < addr+ bytes; ptr += step_size) {
        if (ptr+step_size > addr + bytes)
            step_size = addr + bytes - ptr;
        m_memory_file.seek(ptr);
        chunk = m_memory_file.read(step_size);
        buffer.replace(bytes_read, chunk.size(), chunk);
        bytes_read += chunk.size();
    }    
    detach();
    return bytes_read;
}

int DFInstanceLinux::read_raw(const VIRTADDR &addr, size_t bytes, QByteArray &buffer) {
    ssize_t bytes_read;
    buffer.fill(0, bytes); // zero our buffer

    struct iovec local_iov[1];
    struct iovec remote_iov[1];
    local_iov[0].iov_base = buffer.data();
    remote_iov[0].iov_base = (void *)addr;
    local_iov[0].iov_len = remote_iov[0].iov_len = bytes;

    bytes_read = process_vm_readv(m_pid, local_iov, 1, remote_iov, 1, 0);
    if (bytes_read == -1) {
        if (errno == ENOSYS) {
            return read_raw_ptrace(addr, bytes, buffer);
        } else {
            error(0, errno, "error reading %u bytes from 0x%0u to %p", bytes, addr, buffer.data());
        }
    }

    return bytes_read;
}

int DFInstanceLinux::write_raw_ptrace(const VIRTADDR &addr, const size_t &bytes,
                                      void *buffer) {
    // try to attach, will be ignored if we're already attached
    attach();

    /* Since most kernels won't let us write to /proc/<pid>/mem, we have to poke
     * out data in n bytes at a time. Good thing we read way more than we write.
     *
     * On x86-64 systems, the size that POKEDATA writes is 8 bytes instead of
     * 4, so we need to use the sizeof( long ) as our step size.
     */

    // TODO: Should probably have a global define of word size for the
    // architecture being compiled on. For now, sizeof(long) is consistent
    // on most (all?) linux systems so we'll keep this.
    uint stepsize = sizeof( long );
    uint bytes_written = 0; // keep track of how much we've written
    uint steps = bytes / stepsize;
    if (bytes % stepsize)
        steps++;
    LOGD << "WRITE_RAW: WILL WRITE" << bytes << "bytes over" << steps << "steps, with stepsize " << stepsize;

    // we want to make sure that given the case where (bytes % stepsize != 0) we don't
    // clobber data past where we meant to write. So we're first going to read
    // the existing data as it is, and then write the changes over the existing
    // data in the buffer first, then write the buffer with stepsize bytes at a time 
    // to the process. This should ensure no clobbering of data.
    QByteArray existing_data(steps * stepsize, 0);
    read_raw(addr, (steps * stepsize), existing_data);
    LOGD << "WRITE_RAW: EXISTING OLD DATA     " << existing_data.toHex();

    // ok we have our insurance in place, now write our new junk to the buffer
    memcpy(existing_data.data(), buffer, bytes);
    QByteArray tmp2(existing_data);
    LOGD << "WRITE_RAW: EXISTING WITH NEW DATA" << tmp2.toHex();

    // ok, now our to be written data is in part or all of the exiting data buffer
    long tmp_data;
    for (uint i = 0; i < steps; ++i) {
        int offset = i * stepsize;
        // for each step write a single word to the child
        memcpy(&tmp_data, existing_data.mid(offset, stepsize).data(), stepsize);
        QByteArray tmp_data_str((char*)&tmp_data, stepsize);
        LOGD << "WRITE_RAW:" << hex << addr + offset << "HEX" << tmp_data_str.toHex();
        if (ptrace(PTRACE_POKEDATA, m_pid, addr + offset, tmp_data) != 0) {
            perror("write word");
            break;
        } else {
            bytes_written += stepsize;
        }
        long written = ptrace(PTRACE_PEEKDATA, m_pid, addr + offset, 0);
        QByteArray foo((char*)&written, stepsize);
        LOGD << "WRITE_RAW: WE APPEAR TO HAVE WRITTEN" << foo.toHex();
    }
    // attempt to detach, will be ignored if we're several layers into an attach chain
    detach();
    // tell the caller how many bytes we wrote
    return bytes_written;
}

int DFInstanceLinux::write_raw(const VIRTADDR &addr, const size_t &bytes,
                               void *buffer) {
    ssize_t bytes_written;

    struct iovec local_iov[1];
    struct iovec remote_iov[1];
    local_iov[0].iov_base = buffer;
    remote_iov[0].iov_base = (void *)addr;
    local_iov[0].iov_len = remote_iov[0].iov_len = bytes;

    bytes_written = process_vm_writev(m_pid, local_iov, 1, remote_iov, 1, 0);
    if (bytes_written == -1) {
        if (errno == ENOSYS) {
            return write_raw_ptrace(addr, bytes, buffer);
        } else {
            perror("process_vm_writev");
        }
    }

    LOGD << "WROTE" << bytes << "bytes from" << buffer << "to" << addr;

    // tell the caller how many bytes we wrote
    return bytes_written;
}

bool DFInstanceLinux::find_running_copy(bool connect_anyway) {
    // find PID of DF
    TRACE << "attempting to find running copy of DF by executable name";
    QProcess *proc = new QProcess(this);
    QStringList args;
    args << "dwarfort.exe"; // 0.31.04 and earlier
    args << "Dwarf_Fortress"; // 0.31.05+
    proc->start("pidof", args);
    proc->waitForFinished(1000);
    if (proc->exitCode() == 0) { //found it
        QByteArray out = proc->readAllStandardOutput();
        QStringList str_pids = QString(out).split(" ");
        str_pids.sort();
        if(str_pids.count() > 1){
            m_pid = QInputDialog::getItem(0, tr("Warning"),tr("Multiple Dwarf Fortress processes found, please choose the process to use."),str_pids,str_pids.count()-1,false).toInt();
        }else{
            m_pid = str_pids.at(0).toInt();
        }

        m_memory_file.setFileName(QString("/proc/%1/mem").arg(m_pid));

        TRACE << "USING PID:" << m_pid;
    } else {
        QMessageBox::warning(0, tr("Warning"),
                             tr("Unable to locate a running copy of Dwarf "
                                "Fortress, are you sure it's running?"));
        LOGW << "can't find running copy";
        m_is_ok = false;
        return m_is_ok;
    }

    m_inject_addr = unsigned(-1);
    m_alloc_start = 0;
    m_alloc_end = 0;

    map_virtual_memory();

    //qDebug() << "LOWEST ADDR:" << hex << lowest_addr;


    //DUMP LIST OF MEMORY RANGES
    /*
    QPair<uint, uint> tmp_pair;
    foreach(tmp_pair, m_regions) {
        LOGD << "RANGE start:" << hex << tmp_pair.first << "end:" << tmp_pair.second;
    }*/

    VIRTADDR m_base_addr = read_addr(m_lowest_address + 0x18);
    LOGD << "base_addr:" << m_base_addr << "HEX" << hex << m_base_addr;
    m_is_ok = m_base_addr > 0;

    uint checksum = calculate_checksum();
    LOGI << "DF's checksum is" << hexify(checksum);
    if (m_is_ok) {
        m_layout = get_memory_layout(hexify(checksum).toLower(), !connect_anyway);
    }

    //Get dwarf fortress directory
    m_df_dir = QDir(QFileInfo(QString("/proc/%1/cwd").arg(m_pid)).symLinkTarget());
    LOGI << "Dwarf fortress path:" << m_df_dir.absolutePath();

    return m_is_ok || connect_anyway;
}

void DFInstanceLinux::map_virtual_memory() {
    // destroy existing segments
    foreach(MemorySegment *seg, m_regions) {
        delete(seg);
    }
    m_regions.clear();
    m_executable = NULL;

    if (!m_is_ok)
        return;

    // scan the maps to populate known regions of memory
    QFile f(QString("/proc/%1/maps").arg(m_pid));
    if (!f.open(QIODevice::ReadOnly)) {
        LOGE << "Unable to open" << f.fileName();
        return;
    }
    TRACE << "opened" << f.fileName();
    QByteArray line;
    m_lowest_address = 0xFFFFFFFF;
    m_highest_address = 0;
    uint start_addr = 0;
    uint end_addr = 0;
    bool ok;

    QRegExp rx("^([a-f\\d]+)-([a-f\\d]+)\\s([rwxsp-]{4})\\s+[\\d\\w]{8}\\s+[\\d\\w]{2}:[\\d\\w]{2}\\s+(\\d+)\\s*(.+)\\s*$");
    do {
        line = f.readLine();
        // parse the first line to see find the base
        if (rx.indexIn(line) != -1) {
            //LOGD << "RANGE" << rx.cap(1) << "-" << rx.cap(2) << "PERMS" <<
            //        rx.cap(3) << "INODE" << rx.cap(4) << "PATH" << rx.cap(5);
            start_addr = rx.cap(1).toUInt(&ok, 16);
            end_addr = rx.cap(2).toUInt(&ok, 16);
            QString perms = rx.cap(3).trimmed();
            int inode = rx.cap(4).toInt();
            QString path = rx.cap(5).trimmed();

            //LOGD << "RANGE" << hex << start_addr << "-" << end_addr << perms
            //        << inode << "PATH >" << path << "<";
            bool keep_it = false;
            bool main_file = false;
            if (path.contains("[heap]") || path.contains("[stack]") || path.contains("[vdso]"))  {
                keep_it = true;
            } else if (perms.contains("r") && inode && path == QFile::symLinkTarget(QString("/proc/%1/exe").arg(m_pid))) {
                keep_it = true;
                main_file = true;
            } else {
                keep_it = path.isEmpty();
            }
            // uncomment to search HEAP only
            //keep_it = path.contains("[heap]");
            keep_it = true;

            if (keep_it && end_addr > start_addr) {
                MemorySegment *segment = new MemorySegment(path, start_addr, end_addr);
                TRACE << "keeping" << segment->to_string();
                m_regions << segment;
                if (start_addr < m_lowest_address)
                    m_lowest_address = start_addr;
                else if (end_addr > m_highest_address)
                    m_highest_address = end_addr;
                if (main_file && !m_executable && perms.contains("x")) {
                    m_executable = segment;
                    LOGD << "executable" << segment->to_string();
                }
            }
            if (path.contains("[heap]")) {
                //LOGD << "setting heap start address at" << hex << start_addr;
                m_heap_start_address = start_addr;
            }
        }
    } while (!line.isEmpty());
    f.close();

}

/* Support for executing system calls in the context of the game process. */

static const int injection_size = 4;

static const char nop_code_bytes[injection_size] = {
    /* This is the byte pattern used to pad function
       addresses to multiples of 16 bytes. It consists
       of RET and a sequence of NOPs. The NOPs are not
       supposed to be used, so they can be overwritten. */
    static_cast<char>(0xC3), static_cast<char>(0x90), static_cast<char>(0x90), static_cast<char>(0x90)
};

static QByteArray nop_code(nop_code_bytes, injection_size);

static const char injection_code_bytes[injection_size] = {
    /* This is the injected pattern. It keeps the
       original RET, but adds:
           INT 80h
           INT 3h (breakpoint) */
    static_cast<char>(0xC3), static_cast<char>(0xCD), static_cast<char>(0x80), static_cast<char>(0xCC)
};

static QByteArray injection_code(injection_code_bytes, injection_size);

VIRTADDR DFInstanceLinux::find_injection_address()
{
    if (m_inject_addr != unsigned(-1))
        return m_inject_addr;

    if (!m_executable)
        return m_inject_addr = 0;

    int step = 0x8000; // 32K steps
    VIRTADDR pos = m_executable->start_addr + step; // skip first

    // This loop is expected to succeed on the first try:
    for (; pos < m_executable->end_addr; pos += step)
    {
        int size = m_executable->end_addr - pos;
        if (step < size) size = step;
        QByteArray buf;
        read_raw(pos, size, buf);

        // Try searching for existing injection code
        int offset = buf.indexOf(injection_code);
        // Try searching for empty space
        if (offset < 0)
            offset = buf.indexOf(nop_code);

        if (offset >= 0) {
            m_inject_addr = pos + offset;
            LOGD << "injection point found at" << hex << m_inject_addr;
            return m_inject_addr;
        }
    }

    return m_inject_addr = 0;
}

#if __WORDSIZE == 64
#define R_ESP  rsp
#define R_EIP  rip
#define R_ORIG_EAX orig_rax
#define R_SCID rax
#define R_ARG0 rbx
#define R_ARG1 rcx
#define R_ARG2 rdx
#define R_ARG3 rsi
#define R_ARG4 rdi
#define R_ARG5 rbp
#else
#define R_ESP  esp
#define R_EIP  eip
#define R_ORIG_EAX orig_eax
#define R_SCID eax
#define R_ARG0 ebx
#define R_ARG1 ecx
#define R_ARG2 edx
#define R_ARG3 esi
#define R_ARG4 edi
#define R_ARG5 ebp
#endif

qint32 DFInstanceLinux::remote_syscall(int syscall_id,
     qint32 arg0, qint32 arg1, qint32 arg2,
     qint32 arg3, qint32 arg4, qint32 arg5)
{
    /* Find the injection place; on failure bail out */
    VIRTADDR inj_addr = find_injection_address();
    if (!inj_addr)
        return -1;

    /* Save the current value of the main thread registers */
    struct user_regs_struct saved_regs, work_regs;

    if (ptrace(PTRACE_GETREGS, m_pid, 0, &saved_regs) == -1) {
        perror("ptrace getregs");
        LOGE << "Could not retrieve register information";
        return -1;
    }

    /* Prepare the injected code */
    QByteArray inj_area_save;
    if (read_raw(inj_addr, injection_size, inj_area_save) < injection_size ||
        write_raw(inj_addr, injection_size, (void*)injection_code_bytes) < injection_size) {
        LOGE << "Could not prepare the injection area";
        return -1;
    }

    /* Prepare the registers */
    VIRTADDR jump_eip = inj_addr+1; // skip the first RET

    work_regs = saved_regs;
    work_regs.R_EIP = jump_eip;
    work_regs.R_ORIG_EAX = -1; // clear the interrupted syscall state
    work_regs.R_SCID = syscall_id;
    work_regs.R_ARG0 = arg0;
    work_regs.R_ARG1 = arg1;
    work_regs.R_ARG2 = arg2;
    work_regs.R_ARG3 = arg3;
    work_regs.R_ARG4 = arg4;
    work_regs.R_ARG5 = arg5;

    /* Upload the registers. Note that after this point,
       if this process crashes before the context is
       restored, the game will immediately crash too. */
    if (ptrace(PTRACE_SETREGS, m_pid, 0, &work_regs) == -1) {
        perror("ptrace setregs");
        LOGE << "Could not set register information";
        return -1;
    }

    /* Run the thread until the breakpoint is reached */
    int status;
    if (ptrace(PTRACE_CONT, m_pid, 0, SIGCONT) != -1) {
        status = wait_for_stopped();
        // If the process is stopped for some reason, restart it
        while (WSTOPSIG(status) == SIGSTOP &&
               ptrace(PTRACE_CONT, m_pid, 0, SIGCONT) != -1)
            status = wait_for_stopped();
        if (WSTOPSIG(status) != SIGTRAP) {
            LOGE << "Stopped on" << WSTOPSIG(status) << "instead of SIGTRAP";
        }
    } else {
        LOGE << "Failed to run the call.";
    }

    /* Retrieve registers with the syscall result. */
    if (ptrace(PTRACE_GETREGS, m_pid, 0, &work_regs) == -1) {
        perror("ptrace getregs");
        LOGE << "Could not retrieve register information after stepping";
    }

    /* Restore the registers. */
    if (ptrace(PTRACE_SETREGS, m_pid, 0, &saved_regs) == -1) {
        perror("ptrace setregs");
        LOGE << "Could not restore register information";
    }

    /* Defuse the pending signal state:
       If this process crashes before it officially detaches
       from the game, the last signal will be delivered to it.
       If the signal is SIGSTOP, nothing irreversible happens.
       SIGTRAP on the other hand will crash it, losing all data.
       To avoid it, send SIGSTOP to the thread, and resume it
       to immediately be stopped again.
    */
    syscall(SYS_tkill, m_pid, SIGSTOP);

    if (ptrace(PTRACE_CONT, m_pid, 0, 0) == -1 ||
        (status = wait_for_stopped(), WSTOPSIG(status)) != SIGSTOP) {
        LOGE << "Failed to restore thread and stop.";
    }

    /* Restore the modified injection area (not really necessary,
       since it is supposed to be inside unused padding, but...) */
    if (write_raw(inj_addr, injection_size, inj_area_save.data()) < injection_size) {
        LOGE << "Could not restore the injection area";
    }

    if (VIRTADDR(work_regs.R_EIP) == jump_eip+3)
        return work_regs.R_SCID;
    else {
        LOGE << "Single step failed: EIP" << hex << work_regs.R_EIP << "; expected" << jump_eip+3;
        return -4095;
    }
}

/* Memory allocation for strings using remote mmap. */

VIRTADDR DFInstanceLinux::mmap_area(VIRTADDR start, int size) {
    VIRTADDR return_value = remote_syscall(192/*mmap2*/, start, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    // Raw syscalls can't use errno, so the error is in the result.
    // No valid return value can be in the -4095..-1 range.
    if (int(return_value) < 0 && int(return_value) >= -4095) {
        LOGE << "Injected MMAP failed with error: " << -return_value;
        return_value = -1;
    }

    return return_value;
}

VIRTADDR DFInstanceLinux::alloc_chunk(int size) {
    if (size > 1048576 || size <= 0)
        return 0;

    if ((m_alloc_end - m_alloc_start) < size) {
        int apages = (size*2 + 4095)/4096;
        int asize = apages*4096;

        // Try to request contiguous allocation as a hint
        VIRTADDR new_block = mmap_area(m_alloc_end, asize);
        if (new_block == VIRTADDR(-1))
            return 0;

        if (new_block != m_alloc_end)
            m_alloc_start = new_block;
        m_alloc_end = new_block + asize;
    }

    VIRTADDR rv = m_alloc_start;
    m_alloc_start += size;
    return rv;
}

VIRTADDR DFInstanceLinux::get_string(const QString &str) {
    if (m_string_cache.contains(str))
        return m_string_cache[str];

    CP437Codec *c = new CP437Codec();
    QByteArray data = c->fromUnicode(str);

    STLStringHeader header;
    header.capacity = header.length = data.length();
    header.refcnt = 1000000; // huge refcnt to avoid dealloc

    QByteArray buf((char*)&header, sizeof(header));
    buf.append(data);
    buf.append(char(0));

    VIRTADDR addr = alloc_chunk(buf.length());
    if (addr) {
        write_raw(addr, buf.length(), buf.data());
        addr += sizeof(header);
    }

    return m_string_cache[str] = addr;
}

bool DFInstance::authorize(){
    return true;
}
