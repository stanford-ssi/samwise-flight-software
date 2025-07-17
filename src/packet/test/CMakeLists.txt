if(PROFILE STREQUAL "TEST")
    # Enable testing
    enable_testing()
    
    # Packet test executable
    add_executable(packet_test packet_test.c)
    target_link_libraries(packet_test packet)
    target_include_directories(packet_test PUBLIC
        "${PROJECT_SOURCE_DIR}/src/packet"
        "${PROJECT_SOURCE_DIR}/lib/tinycrypt/lib/include"
    )
    
    # Add test
    add_test(NAME PacketTest COMMAND packet_test)
endif()