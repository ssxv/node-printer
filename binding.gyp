{
  "targets": [
    {
      "target_name": "node_printer",
      "conditions": [
        [
          "OS=='win'",
          {
            "sources": [
              "src/native/addon.cpp",
              "src/native/errors.cpp"
            ]
          }
        ],
        [
          "OS!='win'",
          {
            "sources": [
              "src/native/addon.cpp",
              "src/native/errors.cpp"
            ],
            "libraries": [
              "-lcups"
            ]
          }
        ]
      ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")",
        "src/"
      ],
      "dependencies": [],
      "defines": [
        "NAPI_DISABLE_CPP_EXCEPTIONS"
      ],
      "cflags!": [
        "-fno-exceptions"
      ],
      "cflags_cc!": [
        "-fno-exceptions"
      ],
      "xcode_settings": {
        "GCC_ENABLE_CPP_EXCEPTIONS": "YES",
        "CLANG_CXX_LIBRARY": "libc++",
        "MACOSX_DEPLOYMENT_TARGET": "10.7"
      },
      "msvs_settings": {
        "VCCLCompilerTool": {
          "ExceptionHandling": 1
        }
      }
    }
  ]
}