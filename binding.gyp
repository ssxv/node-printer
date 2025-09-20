{
  "targets": [
    {
      "target_name": "node_printer",
      "conditions": [
        [ "OS=='win'", {
          "sources": [ "src/node_printer_win.cc" ]
        }],
        [ "OS!='win'", {
          "sources": [ "src/node_printer_posix.cc" ],
          "libraries": [ "-lcups" ]
        }]
      ],
      "include_dirs": [ "<!@(node -p \"require('node-addon-api').include\")" ],
      "dependencies": [],
      "defines": [ "NAPI_DISABLE_CPP_EXCEPTIONS" ],
      "cflags!": [ "-fno-exceptions" ],
      "msvs_settings": {
        "VCCLCompilerTool": {
          "ExceptionHandling": 1
        }
      }
    }
  ]
}
