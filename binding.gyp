{
  "targets": [
    {
      "target_name": "nfcaddon",
      "sources": [ "nfc-addon.cpp" ],
      "libraries": [
        "-lwinscard"
      ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")"
      ],
      "dependencies": [
        "<!(node -p \"require('node-addon-api').gyp\")"
      ],
      "defines": [ 'NAPI_DISABLE_CPP_EXCEPTIONS' ],
      "configurations": {
        "Release": {
          "msvs_settings": {
            "VCCLCompilerTool": {
              "DebugInformationFormat": "0"
            }
          }
        }
      }
    }
  ]
}
