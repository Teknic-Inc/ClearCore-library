SET version=1.0.13

SET src="%LOCALAPPDATA%\Arduino15\packages\ClearCore\hardware\sam\%version%\Teknic\libClearCore\src"
SET inc="%LOCALAPPDATA%\Arduino15\packages\ClearCore\hardware\sam\%version%\Teknic\libClearCore\inc"
SET libClearCore_issues="cppCheck_issues_libClearCore.txt"

SET unitTests="%LOCALAPPDATA%\Arduino15\packages\ClearCore\hardware\sam\%version%\Teknic\libClearCore\UnitTests\src"
SET unitTests_issues="cppCheck_issues_unitTests.txt"

SET examples="%LOCALAPPDATA%\Arduino15\packages\ClearCore\hardware\sam\%version%\Teknic\Atmel_Examples"
SET examples_issues="cppCheck_issues_examples.txt"

SET arduino_src="%LOCALAPPDATA%\Arduino15\packages\ClearCore\hardware\sam\%version%\cores\arduino"
SET arduino_issues="cppCheck_issues_arduino.txt"

SET cppCheck="C:\Program Files\Cppcheck\cppcheck.exe"

mkdir clearcore_build_dir

%cppCheck% -I "%inc%" --cppcheck-build-dir="clearcore_build_dir" --enable=all --rule-file="configuration\cppcheck_rules.xml" --output-file="%libClearCore_issues%" --suppress-xml="configuration\cppcheck_suppressions.xml" "%src%"

%cppCheck% --cppcheck-build-dir="clearcore_build_dir" --enable=all --rule-file="configuration\cppcheck_rules.xml" --output-file="%unitTests_issues%" --suppress-xml="configuration\cppcheck_suppressions.xml" "%unitTests%"

%cppCheck% --cppcheck-build-dir="clearcore_build_dir" --enable=all --rule-file="configuration\cppcheck_rules.xml" --output-file="%examples_issues%" --suppress-xml="configuration\cppcheck_suppressions.xml" "%examples%"

%cppCheck% --cppcheck-build-dir="clearcore_build_dir" --enable=all --rule-file="configuration\cppcheck_rules.xml" --output-file="%arduino_issues%" --suppress-xml="configuration\cppcheck_suppressions.xml" "%arduino_src%"

rmdir /s /q clearcore_build_dir
