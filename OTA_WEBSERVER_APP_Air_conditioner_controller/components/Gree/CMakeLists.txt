set(component_srcs "src/ir_builder_rmt_YB0F2.c"
                   "src/ir_parser_rmt_YAPOF3.c")

idf_component_register(SRCS "${component_srcs}"
                       INCLUDE_DIRS "include"
                       PRIV_INCLUDE_DIRS ""
                       PRIV_REQUIRES "driver"
                       REQUIRES "")
