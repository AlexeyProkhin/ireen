import qbs.base 1.0

StaticLibrary {
    name: "k8json"
    destination: "lib"

    Depends { name: "cpp" }
    Depends { name: "qt"; submodules: 'core' }

    cpp.defines: [
        "K8JSON_INCLUDE_GENERATOR",
        "K8JSON_INCLUDE_COMPLEX_GENERATOR",
        "K8JSON_LIB_MAKEDLL"
    ]

    files: [
        "k8json/k8json.h",
        "k8json/k8json.cpp"
    ]

    ProductModule {
        Depends { name: "cpp" }
        cpp.includePaths: product.path
    }
}

