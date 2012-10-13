import qbs.base 1.0

StaticLibrary {
    name: "hmac"
    destination: "lib"

    Depends { name: "cpp" }

    files: "hmac/*.c"

    ProductModule {
        Depends { name: "cpp" }
        cpp.defines: "OSCAR_USE_3RDPARTY_HMAC"
    }
}
