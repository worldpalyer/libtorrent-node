{
    "targets": [
        {
            "variables": {
                "other_cflags%": "",
                "other_lflags%": "",
                "DEPS_DIR%": "../deps"
            },
            'cflags_cc!': ['-fexceptions'],
            'target_name': 'torrentnode',
            'defines': [
                "BOOST_ALL_NO_LIB",
            ],
            'include_dirs': [
                '/usr/local/include',
                '/usr/include',
                '<(DEPS_DIR)\include'
            ],
            'library_dirs': [
                '/usr/local/lib',
                '/usr/lib',
                '<(DEPS_DIR)\lib'
            ],
            'sources': [
                'src/common.h',
                'src/hash.hpp',
                'src/hash.cpp',
                'src/plugins.hpp',
                'src/plugins.cpp',
            ],
            "libraries":[
                '-lboost_system',
                "-lboost_iostreams",
                "-lboost_filesystem",
                "-lboost_thread",
                "-ltorrent-rasterbar",
            ],
            'conditions': [
                ['OS=="mac"', {
                    'xcode_settings': {
                        'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
                        'GCC_ENABLE_CPP_RTTI': 'YES',
                        'MACOSX_DEPLOYMENT_TARGET': '10.11',
                        'OTHER_CFLAGS': [
                            "<(other_cflags)",
                        ],
                        'OTHER_LDFLAGS': [
                            "-stdlib=libc++",
                            "-framework CoreFoundation",
                            "-framework SystemConfiguration",
                            "-L/usr/local/lib",
                            "-Wl,-rpath,.",
                            "<(other_cflags)",
                        ],
                    },
                }],
                ["OS=='win'", {
                    'defines': [
                        "LIBED2K_LINKING_SHARED",
                        "NDEBUG",
                        "WITH_SHIPPED_GEOIP_H", "BOOST_LOG_DONOT_USE_WCHAR_T", "LIBED2K_USE_BOOST_DATE_TIME", "BOOST_ASIO_SEPARATE_COMPILATION", "BOOST_ASIO_ENABLE_CANCELIO", "_WIN32_WINNT=0x0601", "WIN32_LEAN_AND_MEAN", "BOOST_ASIO_DISABLE_HANDLER_HOOKS",
                    ],
                    "configurations": {
                        "Debug": {
                            "msvs_settings": {
                                "VCCLCompilerTool": {
                                    "ExceptionHandling": "0",
                                    "AdditionalOptions": [
                                        "/MP /EHsc"
                                    ]
                                },
                                "VCLibrarianTool": {
                                    "AdditionalOptions": [
                                        "/LTCG"
                                    ]
                                },
                                "VCLinkerTool": {
                                    "LinkTimeCodeGeneration": 1,
                                    "LinkIncremental": 1,
                                    "AdditionalLibraryDirectories": [
                                        "../external/thelibrary/lib/debug"
                                    ]
                                }
                            }
                        },
                        "Release": {
                            "msvs_settings": {
                                "VCCLCompilerTool": {
                                    "RuntimeLibrary": 0,
                                    "Optimization": 3,
                                    "FavorSizeOrSpeed": 1,
                                    "InlineFunctionExpansion": 2,
                                    "WholeProgramOptimization": "true",
                                    "OmitFramePointers": "true",
                                    "EnableFunctionLevelLinking": "true",
                                    "EnableIntrinsicFunctions": "true",
                                    "RuntimeTypeInfo": "false",
                                    "ExceptionHandling": "0",
                                    "AdditionalOptions": [
                                        "/MP /EHsc"
                                    ]
                                },
                                "VCLibrarianTool": {
                                    "AdditionalOptions": [
                                        "/LTCG"
                                    ]
                                },
                                "VCLinkerTool": {
                                    "LinkTimeCodeGeneration": 1,
                                    "OptimizeReferences": 2,
                                    "EnableCOMDATFolding": 2,
                                    "LinkIncremental": 1,
                                    "AdditionalLibraryDirectories": [
                                        "../external/thelibrary/lib/release"
                                    ]
                                }
                            }
                        }
                    }
                }],
                ['OS=="linux"', {
                    'cflags_cc': ['-frtti'],
                    'ldflags': [
                        "-L/usr/local/lib",
                        '-lboost_system',
                        "-lboost_iostreams",
                        "-lboost_filesystem",
                        "-lboost_thread",
                        "-lemulex",
                        "<(other_cflags)",
                    ],
                }],
            ],
        }
    ]
}
