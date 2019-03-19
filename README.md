## Configure

Run `npm run configure`

For CLion, you need to specify the location of the header files by setting `CMAKE_JS_INC` (otherwise it can't find node_api.h, although it will build with `npm run compile`).
To get the value, run `./node_modules/.bin/cmake-js print-configure`.
Copy the cmake option and value (e.g. `-DCMAKE_JS_INC="/Users/ncbradley/.cmake-js/node-x64/v8.9.4/include/node"`) to the cmake options in CLion (Preferences | Build, Execution, Deployment | CMake)