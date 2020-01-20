# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES st7735 pcf8563)

set(COMPONENT_SRCS "main.c" "board.c" "sleep_timeout.c" "display.c")
set(COMPONENT_ADD_INCLUDEDIRS "")

register_component()
