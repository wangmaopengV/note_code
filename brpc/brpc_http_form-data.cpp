// Copyright (c) 2014 Baidu, Inc.
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// - Access pb services via HTTP
//   ./http_client http://www.foo.com:8765/EchoService/Echo -d '{"message":"hello"}'
// - Access builtin services
//   ./http_client http://www.foo.com:8765/vars/rpc_server*
// - Access www.foo.com
//   ./http_client www.foo.com

#include <gflags/gflags.h>
#include <butil/logging.h>
#include <brpc/channel.h>

DEFINE_string(filename, "", "file name");
DEFINE_int32(timeout_ms, 1000, "RPC timeout in milliseconds");
DEFINE_int32(max_retry, 3, "Max retries(not including the first RPC)"); 
DEFINE_string(protocol, "http", "http or h2c");

namespace brpc {
DECLARE_bool(http_verbose);
}

int main(int argc, char* argv[]) {
    // Parse gflags. We recommend you to use gflags as well.
    google::ParseCommandLineFlags(&argc, &argv, true);

    if (argc != 2) {
        
        LOG(ERROR) << "Usage: ./http_client \"http(s)://www.foo.com\"";
        return -1;
    }
    char* url = argv[1];
    
    // A Channel represents a communication line to a Server. Notice that 
    // Channel is thread-safe and can be shared by all threads in your program.
    brpc::Channel channel;
    brpc::ChannelOptions options;
    options.protocol = FLAGS_protocol;
    options.timeout_ms = FLAGS_timeout_ms/*milliseconds*/;
    options.max_retry = FLAGS_max_retry;

    // Initialize the channel, NULL means using default options. 
    // options, see `brpc/channel.h'.
    if (channel.Init(url, &options) != 0) {
        
        LOG(ERROR) << "Fail to initialize channel";
        return -1;
    }

    // We will receive response synchronously, safe to put variables
    // on stack.
    brpc::Controller cntl;

    cntl.http_request().uri() = url;
	
    std::string file1;
    FILE *fp = fopen(FLAGS_filename.c_str(), "rb");
    if(!fp){
		
         std::cerr << "filename is empty!" << std::endl;	
         return -1;	
    }
	
    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
	
    file1.resize(size);
    fread(&*file1.begin(), 1, size, fp);
    fclose(fp);

    // 这里实现一个入库的操作，上传两个图片入库
    std::string boundary = "-----------18273871837183684";  // 这里boundary 可以随便填，但不能和请求内容有相同，所以尽量长点
    std::string content_type = "multipart/form-data; boundary=" + boundary;
    cntl.http_request().set_content_type(content_type);
    cntl.http_request().set_method(brpc::HTTP_METHOD_POST);
    
    butil::IOBufBuilder os;

#define PUT_LINE os.buf().push_back(0x0d); os.buf().push_back(0x0a)
#define PUT_OVER os.buf().push_back('-'); os.buf().push_back('-'); PUT_LINE

    // 第一个文件
    // 1. personId1 
    os << boundary; PUT_LINE;
    os << "Content-Disposition: form-data; name=\"Type\""; PUT_LINE;
    PUT_LINE;
    os << 2; PUT_LINE;

    // 2. imageId1
    os << boundary; PUT_LINE;
    os << "Content-Disposition: form-data; name=\"Functions\""; PUT_LINE;
    PUT_LINE;
    os << "200,201,202,203,204,205"; PUT_LINE;

    // 3. image1
    os << boundary; PUT_LINE;
    os << "Content-Disposition: form-data; name=\"Image1\"; filename=\"timg.jpeg\""; PUT_LINE;
    os << "Content-Type: image/jpeg"; PUT_LINE; // 如果不知道文件类型，Content-Type这里写： application/octet-stream
    PUT_LINE;
    os.buf().append(file1); PUT_LINE;

    // 4. image2
     os << boundary; PUT_LINE;
     os << "Content-Disposition: form-data; name=\"Image2\"; filename=\"timg.jpeg\""; PUT_LINE;
     os << "Content-Type: image/jpeg"; PUT_LINE; // 如果不知道文件类型，Content-Type这里写： application/octet-stream
     PUT_LINE;
     os.buf().append(file1); PUT_LINE;

    // 最后一步需要再写一次boundary
    os << boundary; PUT_OVER;

    os.move_to(cntl.request_attachment());

    // Because `done'(last parameter) is NULL, this function waits until
    // the response comes back or error occurs(including timedout).
    channel.CallMethod(NULL, &cntl, NULL, NULL, NULL);
    if (cntl.Failed()) {

        std::cerr << cntl.ErrorText() << std::endl;
        return -1;
    }

    // If -http_verbose is on, brpc already prints the response to stderr.
    if (!brpc::FLAGS_http_verbose) {

        std::cout << cntl.response_attachment().to_string() << std::endl;
    }
    return 0;
}
