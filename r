==2543648== Memcheck, a memory error detector
==2543648== Copyright (C) 2002-2017, and GNU GPL'd, by Julian Seward et al.
==2543648== Using Valgrind-3.18.1 and LibVEX; rerun with -h for copyright info
==2543648== Command: ./webserver sample.yml
==2543648== 
==2543648== Invalid write of size 1
==2543648==    at 0x40D898: Server::handleClientData(int) (multiplexer.cpp:42)
==2543648==    by 0x40F373: Server::handleClientConnectionsForMultipleServers() (multiplexer.cpp:301)
==2543648==    by 0x40673D: main (main.cpp:39)
==2543648==  Address 0x4e06160 is 0 bytes after a block of size 1,024 alloc'd
==2543648==    at 0x4849013: operator new(unsigned long) (in /usr/libexec/valgrind/vgpreload_memcheck-amd64-linux.so)
==2543648==    by 0x413726: std::__new_allocator<unsigned char>::allocate(unsigned long, void const*) (new_allocator.h:137)
==2543648==    by 0x4136C0: __gnu_cxx::__alloc_traits<std::allocator<unsigned char>, unsigned char>::allocate(std::allocator<unsigned char>&, unsigned long) (alloc_traits.h:133)
==2543648==    by 0x41363F: std::_Vector_base<unsigned char, std::allocator<unsigned char> >::_M_allocate(unsigned long) (stl_vector.h:378)
==2543648==    by 0x436EF5: std::vector<unsigned char, std::allocator<unsigned char> >::_M_fill_insert(__gnu_cxx::__normal_iterator<unsigned char*, std::vector<unsigned char, std::allocator<unsigned char> > >, unsigned long, unsigned char const&) (vector.tcc:581)
==2543648==    by 0x43675F: std::vector<unsigned char, std::allocator<unsigned char> >::resize(unsigned long, unsigned char) (stl_vector.h:1053)
==2543648==    by 0x435B3B: Binary_String::Binary_String(unsigned long) (Binary_String.cpp:17)
==2543648==    by 0x40D815: Server::handleClientData(int) (multiplexer.cpp:39)
==2543648==    by 0x40F373: Server::handleClientConnectionsForMultipleServers() (multiplexer.cpp:301)
==2543648==    by 0x40673D: main (main.cpp:39)
==2543648== 
==2543648== 
==2543648== Process terminating with default action of signal 2 (SIGINT)
==2543648==    at 0x4CC9DEA: epoll_wait (epoll_wait.c:30)
==2543648==    by 0x40F292: Server::handleClientConnectionsForMultipleServers() (multiplexer.cpp:289)
==2543648==    by 0x40673D: main (main.cpp:39)
==2543648== 
==2543648== HEAP SUMMARY:
==2543648==     in use at exit: 119,348 bytes in 217 blocks
==2543648==   total heap usage: 2,444 allocs, 2,227 frees, 455,038 bytes allocated
==2543648== 
==2543648== 512 bytes in 1 blocks are definitely lost in loss record 43 of 60
==2543648==    at 0x4849013: operator new(unsigned long) (in /usr/libexec/valgrind/vgpreload_memcheck-amd64-linux.so)
==2543648==    by 0x40D5F9: Server::handleNewConnection(int) (multiplexer.cpp:29)
==2543648==    by 0x40F339: Server::handleClientConnectionsForMultipleServers() (multiplexer.cpp:297)
==2543648==    by 0x40673D: main (main.cpp:39)
==2543648== 
==2543648== 512 bytes in 1 blocks are definitely lost in loss record 44 of 60
==2543648==    at 0x4849013: operator new(unsigned long) (in /usr/libexec/valgrind/vgpreload_memcheck-amd64-linux.so)
==2543648==    by 0x445311: Server::createFileName(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, int) (multipart.cpp:43)
==2543648==    by 0x4458E7: Server::writeData(Binary_String&, int) (multipart.cpp:78)
==2543648==    by 0x4462F2: Server::handlePostRequest(int) (multipart.cpp:160)
==2543648==    by 0x40EB12: Server::handleClientOutput(int) (multiplexer.cpp:185)
==2543648==    by 0x40F3AD: Server::handleClientConnectionsForMultipleServers() (multiplexer.cpp:305)
==2543648==    by 0x40673D: main (main.cpp:39)
==2543648== 
==2543648== 8,712 (520 direct, 8,192 indirect) bytes in 1 blocks are definitely lost in loss record 58 of 60
==2543648==    at 0x4849013: operator new(unsigned long) (in /usr/libexec/valgrind/vgpreload_memcheck-amd64-linux.so)
==2543648==    by 0x444FE8: Server::readFileChunk_post(int) (multipart.cpp:10)
==2543648==    by 0x446253: Server::handlePostRequest(int) (multipart.cpp:156)
==2543648==    by 0x40EB12: Server::handleClientOutput(int) (multiplexer.cpp:185)
==2543648==    by 0x40F3AD: Server::handleClientConnectionsForMultipleServers() (multiplexer.cpp:305)
==2543648==    by 0x40673D: main (main.cpp:39)
==2543648== 
==2543648== LEAK SUMMARY:
==2543648==    definitely lost: 1,544 bytes in 3 blocks
==2543648==    indirectly lost: 8,192 bytes in 1 blocks
==2543648==      possibly lost: 0 bytes in 0 blocks
==2543648==    still reachable: 109,612 bytes in 213 blocks
==2543648==         suppressed: 0 bytes in 0 blocks
==2543648== Reachable blocks (those to which a pointer was found) are not shown.
==2543648== To see them, rerun with: --leak-check=full --show-leak-kinds=all
==2543648== 
==2543648== For lists of detected and suppressed errors, rerun with: -s
==2543648== ERROR SUMMARY: 16 errors from 4 contexts (suppressed: 0 from 0)
