{
  "targets": [
    {
      "target_name": "clr",
      "sources": [
        "binding.gyp",
        "package.json",
        "readme.md",
        "lib/clr.js",
        "src/node-clr.h",
        "src/node-clr.cc",
        "src/Marshal.h",
        "src/Marshal.cc",
        "src/CLRObject.h",
        "src/CLRObject.cc",
        "src/CLRBinder.h",
        "src/CLRBinder.cc",
        "src/V8Function.h",
        "src/V8Function.cc",
		"src/V8Delegate.h",
		"src/V8Delegate.cc",
		"src/JavascriptError.h",
		"src/JavascriptError.cc",
        "test/clr.js",
        "test/clr.node.js"
      ],
      "msvs_settings": {
        "VCCLCompilerTool": {
          "AdditionalOptions": [ "/clr" ],
          "DisableSpecificWarnings": [ "4506" ]
        },
        "VCLinkerTool": {
          "AdditionalOptions": [ "/ignore:4248" ]
        }
      },
      "configurations" : {
        "Release": {
          "msvs_settings": {
            "VCCLCompilerTool": {
              "RuntimeLibrary": 2,
              "RuntimeTypeInfo": "true"
            },
            "VCLinkerTool": {
              "LinkTimeCodeGeneration": 0
            }
          }
        },
        "Debug": {
          "msvs_settings": {
            "VCCLCompilerTool": {
              "RuntimeLibrary": 3,
              "BasicRuntimeChecks": 0,
              "ExceptionHandling": 0
            }
          }
        }
      }
    }
  ]
}
