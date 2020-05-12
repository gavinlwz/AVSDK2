./protobufutil -I=. --cpp_out=. common.proto
./protobufutil -I=. --cpp_out=. YouMeServerValidProtocol.proto
./protobufutil -I=. --cpp_out=. YoumeRunningState.proto
./protobufutil -I=. --cpp_out=. serverlogin.proto
./protobufutil -I=. --cpp_out=. uploadlog.proto
./protobufutil -I=. --cpp_out=. BandwidthControl.proto
find . -name "*.h" -or -name "*.cc" | xargs sed -i "" -e "s/google::/youmecommon::/g"
find . -name "*.h" -or -name "*.cc" | xargs sed -i "" -e "s/namespace\ google/namespace\ youmecommon/g"
