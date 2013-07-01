{
  "targets": [
    {
      "target_name": "clr",
      "sources": [
		"readme.md",
		"lib/clr.js",
		"src/node-clr.h",
		"src/node-clr.cc",
		"src/CLRObject.h",
		"src/CLRObject.cc",
		"src/util.h",
		"src/util.cc",
		"src/V8Callback.h",
		"src/V8Callback.cc",
		"test/test.js" ],
      "configurations" : {
        "Release": {
          "msvs_settings": {
            "VCCLCompilerTool": {
              "RuntimeLibrary": 2,
              "RuntimeTypeInfo": "true",
              "AdditionalOptions": [ "/clr" ],
            }
          }
        },
        "Debug": {
          "msvs_settings": {
            "VCCLCompilerTool": {
              "RuntimeLibrary": 3,
              "BasicRuntimeChecks": 0,
              "ExceptionHandling": 0,
              "AdditionalOptions": [ "/clr" ],
            }
          }
        }
      }
    }
  ]
}
