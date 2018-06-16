cmake_minimum_required(VERSION 3.10)
project(addressbook C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(addressbook
        addressbook.c
        addressbook.h
        application.c
        application.h
        crypto.c
        crypto.h
        csv_parser.c
        csv_parser.h
        interaction.c
        interaction.h
        editline.c
        editline.h
        list.h
        main.c
        termanip.c
        termanip.h
        user.c
        user.h
        util.c
        util.h)
