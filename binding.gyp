{
  "target_defaults": {
    "include_dirs": [
      "<!(node -e \"require('nan')\")"
    ],
    "sources": [
      ".gitignore",
      "binding.gyp",
      "package.json",
      "src/clr.h",
      "src/CLRType.h",
      "src/CLRType.cc",
      "src/CLRObject.h",
      "src/CLRObject.cc",
      "src/Marshal.h",
      "src/Marshal.cc",
      "src/Binder.h",
      "src/Binder.cc",
      "lib/clr.js"
    ],
    "msvs_settings": {
      "VCCLCompilerTool":{
        "AdditionalOptions": ["/clr"]
      },
      "VCLinkerTool": {
        "AdditionalOptions": ["/ignore:4248"]
      }
    }
  },
  "targets": [{
    "target_name": "clr",
    "win_delay_load_hook": "false",
    "sources+": ["src/clr.cc"],
    "configurations": {
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
            "BasicRuntimeChecks": 0
          }
        }
      }
    }
  }, {
    "target_name": "clr_test",
    "win_delay_load_hook": "false",
    "sources+": ["test/clr_test.cc"],
    "configurations": {
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
            "BasicRuntimeChecks": 0
          }
        }
      }
    }
  }]
}
