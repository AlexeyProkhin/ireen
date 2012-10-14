Product {
    name: "ireen"

    property bool useThirdPartyHmac: true
    property bool sslSupport: true

    property string versionMajor: '0'
    property string versionMinor: '1'
    property string versionRelease: '0'
    property string version: versionMajor+'.'+versionMinor+'.'+versionRelease
    
    destination: "lib"
    type: ["dynamiclibrary", "installed_content"]


    Depends { name: "cpp" }
    Depends { name: "Qt.core" }
    Depends { name: "Qt.network" }
    Depends { name: "Qt.gui" }
    Depends { name: "k8json"}
    Depends { name: "qca"; condition: !useThirdPartyHmac}
    Depends { name: "hmac"; condition: useThirdPartyHmac }
    Depends { name: "windows.ws2_32" }

    //cpp.warningLevel: "all"
    cpp.includePaths: [
        ".",
        "core",
        "metainfo",
        "3rdparty"
    ]
    cpp.defines: ["IREEN_BUILD_LIBRARY"]
    cpp.positionIndependentCode: true
    cpp.visibility: 'hidden'

    Properties {
        condition: !useThirdPartyHmac
        cpp.defines: outer.concat("OSCAR_USE_QCA2")
    }
    Properties {
        condition: sslSupport
        cpp.defines: outer.concat("IREEN_SSL_SUPPORT")
    }

    files: [
        "*.h",
        "*.cpp",
        "metainfo/*.h",
        "metainfo/*.cpp",
        "core/*.h",
        "core/*.cpp"
    ]

    //Group {
    //    //sslSupport
    //    condition: sslSupport
    //    files: [
    //        "oscarauth.h",
    //        "oscarauth.cpp",
    //    ]
    //}

    Group {
        //install headers
        qbs.installDir: "include/ireen"
        fileTags: ["install"]
        files: [
            "*[^_][a-z].h",
        ]
    }
    Group {
        qbs.installDir: "include/ireen/core"
        fileTags: ["install"]
        files: [
            "core/*[^_][a-z].h",
        ]
    }
    Group {
        qbs.installDir: "include/ireen/metainfo"
        fileTags: ["install"]
        files: [
            "metainfo/*[^_][a-z].h",
        ]
    }

    ProductModule {
        Depends { name: "cpp" }
        cpp.includePaths: product.buildDirectory + "/include/ireen"
    }
} 
