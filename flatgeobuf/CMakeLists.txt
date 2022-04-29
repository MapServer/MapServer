add_library(flatgeobuf STATIC flatgeobuf_c.cpp geometryreader.cpp packedrtree.cpp)
target_include_directories(flatgeobuf PRIVATE include)
install(TARGETS flatgeobuf)
