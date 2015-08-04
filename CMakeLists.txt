cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)
project(DwarfTherapist)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5 REQUIRED COMPONENTS Qml Widgets)

include(CPack)
include(InstallRequiredSystemLibraries)
include_directories(src src/models src/grid_view src/docks thirdparty/qtcolorpicker-2.6)

if(WIN32)
    set(SOURCES ${SOURCES} DwarfTherapist.rc src/dfinstancewindows.cpp)
    set(LIBS user32 psapi)
    add_definitions(-DNOMINMAX)

#check_dirs.path = $$DESTDIR
#check_dirs.extra = if not exist "$$DESTDIRsharememory_layoutswindows" mkdir "$$DESTDIRsharememory_layoutswindows";

#copy_mem_layouts.path = $$DESTDIR
#copy_mem_layouts.extra = copy /Y "$$PWDsharememory_layoutswindows*" ".$$DESTDIRsharememory_layoutswindows";

#INSTALLS += check_dirs
#INSTALLS += copy_mem_layouts
elseif(UNIX)
    set(SOURCES ${SOURCES} src/dfinstancenix.cpp)
    if(APPLE)
        set(SOURCES ${SOURCES} DwarfTherapist.rc src/dfinstanceosx.mm)
        #ICON = hammer.icns
        #LIBS += -framework Cocoa
        #    LIBS += -framework Carbon
        #    LIBS += -framework Security
        #    LIBS += -framework SecurityFoundation
        #    LIBS += -framework Foundation
        #    LIBS += -framework ApplicationServices
        #    LIBS += -framework Accelerate

        #    log.path = Contents/MacOS/log
        #    QMAKE_BUNDLE_DATA += log

        #    share.path = Contents/MacOS/share
        #    QMAKE_BUNDLE_DATA += share

        #    memory_layouts.path = Contents/MacOS/share/memory_layouts/osx
        #    memory_layouts.files += $$files(share/memory_layouts/osx/*)
    else()
        set(SOURCES ${SOURCES} src/dfinstancelinux.cpp)

        #target.path = $$PREFIX/bin
        #    INSTALLS += target

        #    bin.path = $$PREFIX/bin
        #    bin.files += dist/dwarftherapist
        #    INSTALLS += bin

        #    application.path = $$PREFIX/share/applications
        #    application.files += dist/dwarftherapist.desktop
        #    INSTALLS += application

        #    doc.path = $$PREFIX/share/doc/dwarftherapist
        #    doc.files += LICENSE.txt
        #    doc.files += README.rst
        #    system("printf 'Checking for pdflatex... '; if ! command -v pdflatex; then echo 'not found'; exit 1; fi") {
        #        manual.depends = "$$PWD/doc/Dwarf Therapist.tex" $$PWD/doc/images/*
        #        manual.commands = [ -d doc ] || mkdir doc;
        #        manual.commands += TEXINPUTS=".:$$PWD/doc/images:" pdflatex -output-directory=doc "$<"
        #        manual.target = "doc/Dwarf Therapist.pdf"
        #        QMAKE_EXTRA_TARGETS += manual
        #        POST_TARGETDEPS += "$$manual.target"
        #        doc.files += "$$manual.target"
        #    }
        #    INSTALLS += doc

        #    icon.path = $$PREFIX/share/pixmaps
        #    icon.files += img/dwarftherapist.png
        #    icon.files += img/dwarftherapist.xpm
        #    INSTALLS += icon

        #    memory_layouts.path = $$PREFIX/share/dwarftherapist/memory_layouts/linux
        #    memory_layouts.files += $$files(share/memory_layouts/linux/*)
        #    INSTALLS += memory_layouts
    endif()
else()
    message(FATAL_ERROR "unsupported target")
endif()


# Translation files
#TRANSLATIONS += ./dwarftherapist_en.ts

add_executable(DwarfTherapist WIN32 MACOSX_BUNDLE src/grid_view/cellcolordef.h
    src/roleaspect.h src/itemgenericsubtype.h src/itemtoolsubtype.h
    src/plandetail.h src/viewmanager.cpp
    src/uberdelegate.cpp src/truncatingfilelogger.cpp src/statetableview.cpp
    src/squad.cpp src/skill.cpp src/scriptdialog.cpp src/rotatedheader.cpp
    src/optionsmenu.cpp src/memorylayout.cpp src/mainwindow.cpp src/main.cpp
    src/importexportdialog.cpp src/gridviewdialog.cpp src/gamedatareader.cpp
    src/dwarftherapist.cpp src/dwarfdetailswidget.cpp src/dwarf.cpp
    src/dfinstance.cpp src/customprofession.cpp src/customcolor.cpp
    src/aboutdialog.cpp src/models/dwarfmodelproxy.cpp
    src/models/dwarfmodel.cpp src/grid_view/viewcolumnset.cpp
    src/grid_view/viewcolumn.cpp src/grid_view/traitcolumn.cpp
    src/grid_view/spacercolumn.cpp src/grid_view/skillcolumn.cpp
    src/grid_view/laborcolumn.cpp src/grid_view/currentjobcolumn.cpp
    src/grid_view/happinesscolumn.cpp src/grid_view/gridview.cpp
    src/grid_view/attributecolumn.cpp src/docks/skilllegenddock.cpp
    src/docks/gridviewdock.cpp src/docks/dwarfdetailsdock.cpp
    thirdparty/qtcolorpicker-2.6/qtcolorpicker.cpp
    src/selectparentlayoutdialog.cpp src/word.cpp src/grid_view/flagcolumn.cpp
    src/grid_view/rolecolumn.cpp src/dwarfstats.cpp src/role.cpp
    src/attribute.cpp src/grid_view/weaponcolumn.cpp src/roledialog.cpp
    src/races.cpp src/languages.cpp src/caste.cpp src/reaction.cpp
    src/fortressentity.cpp src/material.cpp src/plant.cpp
    src/docks/preferencesdock.cpp src/laboroptimizer.cpp
    src/optimizereditor.cpp src/laboroptimizerplan.cpp src/preference.cpp
    src/grid_view/professioncolumn.cpp src/iconchooser.cpp
    src/grid_view/highestmoodcolumn.cpp src/trait.cpp
    src/docks/thoughtsdock.cpp src/thought.cpp src/grid_view/trainedcolumn.cpp
    src/unithealth.cpp src/unitwound.cpp src/grid_view/healthcolumn.cpp
    src/docks/healthlegenddock.cpp src/defaultfonts.cpp
    src/grid_view/equipmentcolumn.cpp src/grid_view/itemtypecolumn.cpp
    src/item.cpp src/uniform.cpp src/itemweaponsubtype.cpp src/rolestats.cpp
    src/belief.cpp src/grid_view/superlaborcolumn.cpp src/superlabor.cpp
    src/multilabor.cpp src/grid_view/customprofessioncolumn.cpp
    src/syndrome.cpp src/grid_view/beliefcolumn.cpp src/unitbelief.cpp
    src/histfigure.cpp src/grid_view/unitkillscolumn.cpp
    src/dtstandarditem.cpp src/docks/informationdock.cpp
    src/docks/equipmentoverviewdock.cpp src/rolecalcbase.cpp
    src/itemarmorsubtype.cpp src/flagarray.cpp src/unitemotion.cpp
    src/emotion.cpp src/docks/basetreedock.cpp
    src/grid_view/vieweditordialog.cpp src/grid_view/cellcolors.cpp
    src/grid_view/viewcolumncolors.cpp src/grid_view/viewcolumnsetcolors.cpp
    src/contextmenuhelper.cpp src/eventfilterlineedit.cpp
    src/docks/basedock.cpp src/dwarfjob.cpp src/mood.cpp src/labor.cpp
    src/subthoughttypes.cpp src/itemdefuniform.cpp src/itemsubtype.cpp
    src/emotiongroup.cpp resources.qrc
    ${SOURCES})
target_compile_features(DwarfTherapist PRIVATE cxx_generalized_initializers)
target_link_libraries(DwarfTherapist Qt5::Widgets Qt5::Qml)
