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
#include <QtDebug>

#ifdef Q_OS_WIN
#include <windows.h>
#include <psapi.h>

#include "dfinstance.h"
#include "dfinstancewindows.h"
#include "defines.h"
#include "truncatingfilelogger.h"
#include "dwarf.h"
#include "utils.h"
#include "gamedatareader.h"
#include "memorylayout.h"
#include "win_structs.h"
#include "memorysegment.h"
#include "dwarftherapist.h"
#include "cp437codec.h"

DFInstanceWindows::DFInstanceWindows(QObject* parent)
    : DFInstance(parent)
    , m_proc(0)
{}

DFInstanceWindows::~DFInstanceWindows() {
    if (m_proc) {
        CloseHandle(m_proc);
    }
}

uint DFInstanceWindows::calculate_checksum() {
    BYTE expect_M = read_byte(m_base_addr);
    BYTE expect_Z = read_byte(m_base_addr + 0x1);

    if (expect_M != 'M' || expect_Z != 'Z') {
        qWarning() << "invalid executable";
    }
    uint pe_header = m_base_addr + read_int(m_base_addr + 30 * 2);
    BYTE expect_P = read_byte(pe_header);
    BYTE expect_E = read_byte(pe_header + 0x1);
    if (expect_P != 'P' || expect_E != 'E') {
        qWarning() << "PE header invalid";
    }

    quint32 timestamp = read_addr(pe_header + 4 + 2 * 2);
    QDateTime compile_timestamp = QDateTime::fromTime_t(timestamp);
    LOGI << "Target EXE was compiled at " <<
            compile_timestamp.toString(Qt::ISODate);
    return timestamp;
}

QVector<VIRTADDR> DFInstanceWindows::enumerate_vector(const VIRTADDR &addr) {
    QVector<VIRTADDR> addresses;
    VIRTADDR start = read_addr(addr);
    VIRTADDR end = read_addr(addr + 4);
    if(check_vector(start,end,addr)){
        for (VIRTADDR ptr = start; ptr < end; ptr += 4 ) {
            VIRTADDR a = read_addr(ptr);
            addresses.append(a);
        }
        TRACE << "FOUND" << addresses.size()<< "addresses in vector at" << hexify(addr);
    }else{
        TRACE << "vector at" << hexify(addr) << "failed the check";
    }
    return addresses;
}

QString DFInstanceWindows::read_string(const uint &addr) {
    int len = read_int(addr + memory_layout()->string_length_offset());
    int cap = read_int(addr + memory_layout()->string_cap_offset());
    VIRTADDR buffer_addr = addr + memory_layout()->string_buffer_offset();
    if (cap >= 16)
        buffer_addr = read_addr(buffer_addr);

    if (len > cap || len < 0 || len > 1024) {
#ifdef _DEBUG
        // probably not really a string
        LOGW << "Tried to read a string at" << hex << addr
            << "but it was totally not a string...";
#endif
        return QString();
    }
    Q_ASSERT_X(len <= cap, "read_string",
               "Length must be less than or equal to capacity!");
    Q_ASSERT_X(len >= 0, "read_string", "Length must be >=0!");
    Q_ASSERT_X(len < (1 << 16), "read_string",
               "String must be of sane length!");

    QByteArray buf = get_data(buffer_addr, len);
    CP437Codec *c = new CP437Codec();
    return c->toUnicode(buf);

    //the line below would be nice, but apparently a ~20mb *.icu library is required for that single call to qtextcodec...wtf. really.
    //it's also been pretty bad performance-wise on linux, so it may be best to forget about it entirely
    //return QTextCodec::codecForName("IBM 437")->toUnicode(buf);
}

int DFInstanceWindows::write_string(const VIRTADDR &addr, const QString &str) {
    /*

      THIS IS TOTALLY DANGEROUS

      */

    // TODO, don't write strings longer than 15 characters to the string
    // unless it has already been expanded to a bigger allocation

    int cap = read_int(addr + memory_layout()->string_cap_offset());
    VIRTADDR buffer_addr = addr + memory_layout()->string_buffer_offset();
    if( cap >= 16 )
        buffer_addr = read_addr(buffer_addr);

    int len = qMin<int>(str.length(), cap);
    write_int(addr + memory_layout()->string_length_offset(), len);

    QByteArray data = QTextCodec::codecForName("IBM 437")->fromUnicode(str);
    int bytes_written = write_raw(buffer_addr, len, data.data());
    return bytes_written;
}

int DFInstanceWindows::write_int(const VIRTADDR &addr, const int &val) {
    int bytes_written = 0;
    WriteProcessMemory(m_proc, (LPVOID)addr, &val, sizeof(int),
                       (DWORD*)&bytes_written);
    return bytes_written;
}

int DFInstanceWindows::read_raw(const VIRTADDR &addr, size_t bytes,
                                QByteArray &buffer) {
    buffer.fill(0, bytes);
    int bytes_read = 0;
    ReadProcessMemory(m_proc, (LPCVOID)addr, (char*)buffer.data(),
                      sizeof(BYTE) * bytes, (DWORD*)&bytes_read);
    return bytes_read;
}

int DFInstanceWindows::write_raw(const VIRTADDR &addr, const size_t &bytes,void *buffer) {
    int bytes_written = 0;
    WriteProcessMemory(m_proc, (LPVOID)addr, (void*)buffer,
                       sizeof(uchar) * bytes, (DWORD*)&bytes_written);

    Q_ASSERT(bytes_written == bytes);
    return bytes_written;
}

bool DFInstanceWindows::find_running_copy(bool connect_anyway) {
    LOGI << "attempting to find running copy of DF by window handle";
    m_is_ok = false;

    HWND hwnd = FindWindow(L"OpenGL", L"Dwarf Fortress");
    if (!hwnd)
        hwnd = FindWindow(L"SDL_app", L"Dwarf Fortress");
    if (!hwnd)
        hwnd = FindWindow(NULL, L"Dwarf Fortress");

    if (!hwnd) {
        QMessageBox::warning(0, tr("Warning"),
            tr("Unable to locate a running copy of Dwarf "
            "Fortress, are you sure it's running?"));
        LOGW << "can't find running copy";
        return m_is_ok;
    }
    LOGI << "found copy with HWND: " << hwnd;

    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == 0) {
        return m_is_ok;
    }
    LOGI << "PID of process is: " << pid;
    m_pid = pid;
    m_hwnd = hwnd;

    m_proc = OpenProcess(PROCESS_QUERY_INFORMATION
                         | PROCESS_VM_READ
                         | PROCESS_VM_OPERATION
                         | PROCESS_VM_WRITE, false, m_pid);
    LOGI << "PROC HANDLE:" << m_proc;
    if (m_proc == NULL) {
        LOGE << "Error opening process!" << GetLastError();
    }

    PVOID peb_addr = GetPebAddress(m_proc);
    LOGI << "PEB is at: " << hex << peb_addr;

    QString connection_error = tr("I'm sorry. I'm having trouble connecting to "
                                  "DF. I can't seem to locate the PEB address "
                                  "of the process. \n\nPlease re-launch DF and "
                                  "try again.");
    if (peb_addr == 0){
        QMessageBox::critical(0, tr("Connection Error"), connection_error);
        qCritical() << "PEB address came back as 0";
    } else {
        PEB peb;
        DWORD bytes = 0;
        if (ReadProcessMemory(m_proc, (PCHAR)peb_addr, &peb, sizeof(PEB), &bytes)) {
            LOGI << "read" << bytes << "bytes BASE ADDR is at: " << hex << peb.ImageBaseAddress;
            m_base_addr = (int)peb.ImageBaseAddress;
            m_is_ok = true;
        } else {
            QMessageBox::critical(0, tr("Connection Error"), connection_error);
            qCritical() << "unable to read remote PEB!" << GetLastError();
            m_is_ok = false;
        }
    }

    if (m_is_ok) {
        m_layout = get_memory_layout(hexify(calculate_checksum()).toLower(), !connect_anyway);
    }

    if(!m_is_ok) {
        if(connect_anyway)
            m_is_ok = true;
        else // time to bail
            return m_is_ok;
    }

    m_memory_correction = (int)m_base_addr - 0x0400000;
    LOGI << "base address:" << hexify(m_base_addr);
    LOGI << "memory correction:" << hexify(m_memory_correction);

    map_virtual_memory();

    if (DT->user_settings()->value("options/alert_on_lost_connection", true)
        .toBool() && m_layout && m_layout->is_complete()) {
        m_heartbeat_timer->start(1000); // check every second for disconnection
    }

    char * modName = new char[MAX_PATH];
    DWORD lenModName = 0;
    if ((lenModName = GetModuleFileNameExA(m_proc, NULL, modName, MAX_PATH)) != 0) {
        QString exe_path = QString::fromLocal8Bit(modName, lenModName);
        LOGI << "GetModuleFileNameEx returned: " << exe_path;
        QFileInfo exe(exe_path);
        m_df_dir = exe.absoluteDir();
        LOGI << "Dwarf fortress path:" << m_df_dir.absolutePath();
    }

    m_is_ok = true;
    return m_is_ok;
}

/*! OS specific way of asking the kernel for valid virtual memory pages from
  the DF process. These pages are used to speed up scanning, and validate
  reads from DF's memory. If addresses are not within ranges found by this
  method, they will fail the is_valid_address() method */
void DFInstanceWindows::map_virtual_memory() {
    // destroy existing segments
    foreach(MemorySegment *seg, m_regions) {
        delete(seg);
    }
    m_regions.clear();

    if (!m_is_ok)
        return;

    // start by figuring out what kernel we're talking to
    TRACE << "Mapping out virtual memory";
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    TRACE << "PROCESSORS:" << info.dwNumberOfProcessors;
    TRACE << "PROC TYPE:" << info.wProcessorArchitecture <<
            info.wProcessorLevel <<
            info.wProcessorRevision;
    TRACE << "PAGE SIZE" << info.dwPageSize;
    TRACE << "MIN ADDRESS:" << hexify((uint)info.lpMinimumApplicationAddress);
    TRACE << "MAX ADDRESS:" << hexify((uint)info.lpMaximumApplicationAddress);

    uint start = (uint)info.lpMinimumApplicationAddress;
    uint max_address = (uint)info.lpMaximumApplicationAddress;
    int page_size = info.dwPageSize;
    int accepted = 0;
    int rejected = 0;
    uint segment_start = start;
    uint segment_size = page_size;
    while (start < max_address) {
        MEMORY_BASIC_INFORMATION mbi;
        int sz = VirtualQueryEx(m_proc, (void*)start, &mbi,
                                sizeof(MEMORY_BASIC_INFORMATION));
        if (sz != sizeof(MEMORY_BASIC_INFORMATION)) {
            // incomplete data returned. increment start and move on...
            start += page_size;
            continue;
        }

        segment_start = (uint)mbi.BaseAddress;
        segment_size = (uint)mbi.RegionSize;
        if (mbi.State == MEM_COMMIT
            //&& !(mbi.Protect & PAGE_GUARD)
            && (mbi.Protect & PAGE_EXECUTE_READ ||
                mbi.Protect & PAGE_EXECUTE_READWRITE ||
                mbi.Protect & PAGE_READONLY ||
                mbi.Protect & PAGE_READWRITE ||
                mbi.Protect & PAGE_WRITECOPY)
            ) {
            TRACE << "FOUND READABLE COMMITED MEMORY SEGMENT FROM" <<
                    hexify(segment_start) << "-" <<
                    hexify(segment_start + segment_size) <<
                    "SIZE:" << (segment_size / 1024.0f) << "KB" <<
                    "FLAGS:" << mbi.Protect;
            MemorySegment *segment = new MemorySegment("", segment_start,
                                                       segment_start
                                                       + segment_size);
            segment->is_guarded = mbi.Protect & PAGE_GUARD;
            m_regions << segment;
            accepted++;
        } else {
            TRACE << "REJECTING MEMORY SEGMENT AT" << hexify(segment_start) <<
                     "SIZE:" << (segment_size / 1024.0f) << "KB FLAGS:" <<
                     mbi.Protect;
            rejected++;
        }
        if (mbi.RegionSize)
            start += mbi.RegionSize;
        else
            start += page_size;
    }
    m_lowest_address = 0xFFFFFFFF;
    m_highest_address = 0;
    foreach(MemorySegment *seg, m_regions) {
        if (seg->start_addr < m_lowest_address)
            m_lowest_address = seg->start_addr;
        if (seg->end_addr > m_highest_address)
            m_highest_address = seg->end_addr;
    }
    LOGD << "MEMORY SEGMENT SUMMARY: accepted" << accepted << "rejected" <<
            rejected << "total" << accepted + rejected;
}

bool DFInstance::authorize(){
    return true;
}

#endif
