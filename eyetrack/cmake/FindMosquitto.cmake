# FindMosquitto.cmake
#
# Finds the Eclipse Paho MQTT C++ client library.
#
# Sets:
#   PAHO_MQTT_FOUND          - TRUE if found
#   PAHO_MQTT_CPP_LIBRARY    - C++ library path
#   PAHO_MQTT_C_LIBRARY      - C library path (async)
#   PAHO_MQTT_INCLUDE_DIR    - Include directory
#   PahoMQTT::PahoMQTTCpp   - Imported target (C++)
#
# Hints:
#   PAHO_MQTT_ROOT - Root directory of Paho MQTT installation

find_path(PAHO_MQTT_INCLUDE_DIR
    NAMES mqtt/async_client.h
    HINTS
        ${PAHO_MQTT_ROOT}/include
        /usr/include
        /usr/local/include
)

find_library(PAHO_MQTT_CPP_LIBRARY
    NAMES paho-mqttpp3
    HINTS
        ${PAHO_MQTT_ROOT}/lib
        /usr/lib
        /usr/local/lib
        /usr/lib/x86_64-linux-gnu
        /usr/lib/aarch64-linux-gnu
)

find_library(PAHO_MQTT_C_LIBRARY
    NAMES paho-mqtt3as
    HINTS
        ${PAHO_MQTT_ROOT}/lib
        /usr/lib
        /usr/local/lib
        /usr/lib/x86_64-linux-gnu
        /usr/lib/aarch64-linux-gnu
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Mosquitto
    REQUIRED_VARS PAHO_MQTT_CPP_LIBRARY PAHO_MQTT_C_LIBRARY PAHO_MQTT_INCLUDE_DIR
)

if(Mosquitto_FOUND AND NOT TARGET PahoMQTT::PahoMQTTCpp)
    add_library(PahoMQTT::PahoMQTTC UNKNOWN IMPORTED)
    set_target_properties(PahoMQTT::PahoMQTTC PROPERTIES
        IMPORTED_LOCATION "${PAHO_MQTT_C_LIBRARY}"
    )

    add_library(PahoMQTT::PahoMQTTCpp UNKNOWN IMPORTED)
    set_target_properties(PahoMQTT::PahoMQTTCpp PROPERTIES
        IMPORTED_LOCATION "${PAHO_MQTT_CPP_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${PAHO_MQTT_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES PahoMQTT::PahoMQTTC
    )
endif()

mark_as_advanced(PAHO_MQTT_INCLUDE_DIR PAHO_MQTT_CPP_LIBRARY PAHO_MQTT_C_LIBRARY)
