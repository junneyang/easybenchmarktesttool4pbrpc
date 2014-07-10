#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <cstring>
#include <vector>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <algorithm>
#include <pthread.h>
#include <unistd.h>

#include "google/protobuf/text_format.h"

#include "google/protobuf/stubs/common.h"
#include "sofa/pbrpc/rpc_channel.h"
#include "sofa/pbrpc/common.h"
#include "sofa/pbrpc/rpc_controller.h"

#include <sofa/pbrpc/pbrpc.h>
#include "pbrpc.pb.h"
#include "pbrpc_service.pb.h"


using namespace std;

using lbs::da::openservice::GetBNItemsRequest;
using lbs::da::openservice::GetBNItemsResponse;
using lbs::da::openservice::ItemBytes;

//全局函数声明
//获取当前时间函数，返回单位ms
long getCurrentTime();
//获取测试数据列表
vector<string> get_testdata(string filepath);
//随机获取测试数据
string get_randomdata(vector<string> testdata);
//定义回调函数
void EchoCallback(sofa::pbrpc::RpcController* cntl, lbs::da::openservice::GetBNItemsRequest* request, lbs::da::openservice::GetBNItemsResponse* response, 
				lbs::da::openservice::ItemService_Stub *stub, vector<string> &testdata, bool* params, long start_time);
//定义数据请求函数
void EchoAsynCall(lbs::da::openservice::ItemService_Stub *stub, vector<string> &testdata, bool* params);


//全局变量声明
pthread_mutex_t mutex;//声明全局互斥锁
static int total_req_cnt=0;
static int total_res_cnt=0;
static int total_err_cnt=0;
static int total_nul_cnt=0;

static int below_10=0;
static int between_10_20=0;
static int between_20_30=0;
static int over_30=0;
static long total_res_time=0;

static int stdout_flag=0;

//long int begin_time=0;

//帮助信息
static string help_info="Usage:\n\
./alexsda_easybenchmarktesttool ip:port client_num test_time stdout_flag test_data\n\
ip:port\t\tstring of service ip:port.eg:10.48.55.39:7789\n\
client_num\tnumber of client which could control the pressure.eg:20\n\
test_time\ttotal test time(min).eg:1\n\
stdout_flag\tflag of whether to print stdout,1 represents true,0 represents false.eg:1\n\
test_data\tfile path for test datd.eg:./testdata/test.data\n\n\
Example:\n\
./alexsda_easybenchmarktesttool 10.48.55.39:7789 20 1 0 ./testdata/test.data\n";

long getCurrentTime(){
	struct timeval tv;
	gettimeofday(&tv,NULL);
	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

vector<string> get_testdata(string filepath){
	vector<string> testdata;
	try{
		ifstream in(filepath.c_str());
		for(string tmps;getline(in,tmps);){
			testdata.push_back(tmps);
			//cout<<tmps<<endl;
		}
	}catch(exception const& e){
		cout<<e.what()<<endl;
	}
	
	return testdata;
}

string get_randomdata(vector<string> testdata){
	srand((unsigned)time(NULL));
	random_shuffle(testdata.begin(),testdata.end());
	return (*testdata.begin());
}

std::vector<std::string> split_string(const std::string& str, const std::string& sep){
	std::vector<std::string> ret;
	size_t start = 0;
	size_t str_len = str.size();
	size_t found = std::string::npos;
	while (start < str_len && (found = str.find(sep, start)) != std::string::npos)
	{
		if (found > start)
		{
			std::string sub = str.substr(start, found - start);
			if (!sub.empty())
			{
				ret.push_back(sub);
			}
		}
		start = (found + sep.size());
	}
	if (start < str_len)
	{
		std::string sub = str.substr(start);
		if (!sub.empty())
		{
			ret.push_back(sub);
		}
	}
	return ret;
}


void EchoCallback(sofa::pbrpc::RpcController* cntl, lbs::da::openservice::GetBNItemsRequest* request, lbs::da::openservice::GetBNItemsResponse* response, 
				lbs::da::openservice::ItemService_Stub *stub, vector<string> &testdata, bool* params, long start_time){
	//SLOG(NOTICE, "RemoteAddress=%s", cntl->RemoteAddress().c_str());
	//SLOG(NOTICE, "IsRequestSent=%s", cntl->IsRequestSent() ? "true" : "false");
	/*int current = getCurrentTime();
	if(current >= end){
		*params = true;
	}*/
	
	/*if (cntl->IsRequestSent())
	{
		//SLOG(NOTICE, "LocalAddress=%s", cntl->LocalAddress().c_str());
		//SLOG(NOTICE, "SentBytes=%ld", cntl->SentBytes());
		//cout<<"send success"<<endl;
	}*/
	/*static int below_10=0;
	static int between_10_20=0;
	static int between_20_30=0;
	static int over_30=0;
	static long total_res_time=0;*/
	
	long end_time = getCurrentTime();
	int cost_time = end_time - start_time;
	
	try{
		if (cntl->Failed() || response->items_size() == 0) {
			//SLOG(ERROR, "request failed: %s", cntl->ErrorText().c_str());
			pthread_mutex_lock(&mutex);
			total_err_cnt++;
			pthread_mutex_unlock(&mutex);
		}
		if (response->items_size() == 0) {
			//SLOG(ERROR, "request failed: %s", cntl->ErrorText().c_str());
			pthread_mutex_lock(&mutex);
			total_nul_cnt++;
			pthread_mutex_unlock(&mutex);
		}
		else {
			pthread_mutex_lock(&mutex);
			total_res_cnt++;
			total_res_time+=cost_time;
			if(cost_time<10){
			below_10++;
			}else if(cost_time>=10 && cost_time<20){
				between_10_20++;
			}else if(cost_time>=20 && cost_time<30){
				between_20_30++;
			}else{
				over_30++;
			}
			pthread_mutex_unlock(&mutex);
			
			//SLOG(NOTICE, "request succeed: %s", response->message().c_str());
			if(stdout_flag == 1){
				cout << "response size [" << response->items_size() << "]" << endl;
				for(int i = 0; i < response->items_size(); ++i){
					const ItemBytes &item = response->items(i);

					uint64_t m_poi = strtoull(item.id().c_str(), NULL, 10);
					uint32_t f_value_size = item.value_size();
					float m_score = f_value_size > 0 ? item.value(0) : 0;
					float m_x = f_value_size > 2 ? item.value(2) : 0;
					float m_y = f_value_size > 3 ? item.value(3) : 0;

					string m_poi_name = item.str_value_size() > 0 ? item.str_value(0) : "";
					cout<<"["<<m_poi<<","<<m_score<<","<<m_x<<","<<m_y<<","<<m_poi_name<<endl;
				}
			}
		}
		
		/*pthread_mutex_lock(&mutex);
		long cu_time = getCurrentTime();
		int el_time = cu_time - begin_time;
		
		cout<<"total_req_cnt: "<<total_req_cnt<<endl;
		cout<<"total_res_cnt: "<<total_res_cnt<<endl;
		cout<<"total_nul_cnt: "<<total_nul_cnt<<endl;
		cout<<"total_err_cnt: "<<total_err_cnt<<endl;
		cout<<"QPS(query per second): "<<((double)total_res_cnt/((double)el_time/1000))<<endl<<endl;
		
		cout<<"avg_res_time: "<<(double)total_res_time/(double)total_res_cnt<<" ms"<<endl;
		cout<<"below_10(ms): "<<below_10<<endl;
		cout<<"between_10_20(ms): "<<between_10_20<<endl;
		cout<<"between_20_30(ms): "<<between_20_30<<endl;
		cout<<"over_30(ms): "<<over_30<<endl;
		cout<<"\033[11A";
		cout<<"\033[K";
		
		pthread_mutex_unlock(&mutex);*/
		
		
		EchoAsynCall(stub, testdata, params);
	}catch(exception const& e){
		cout<<e.what()<<endl;
	}
	
	delete cntl;
	delete request;
	delete response;
	//*params = true;
}

void EchoAsynCall(lbs::da::openservice::ItemService_Stub *stub, vector<string> &testdata, bool* params){
	try{
		string randomdata=get_randomdata(testdata);
		//cout<<randomdata<<endl;
		
		//vector<string> input_data=split(randomdata," ");
		vector<string> input_data=split_string(randomdata," ");
		
		/*vector<string>::iterator it;
		for(it=input_data.begin(); it!=input_data.end(); it++){
			cout<<*it<<endl;
		}*/
		
		lbs::da::openservice::GetBNItemsRequest* request = new lbs::da::openservice::GetBNItemsRequest();
		lbs::da::openservice::RequestHeader* header = request->mutable_header();
		header->set_servicekey("key1"); // 头部固定的字符串，laplace暂时没检查
		header->set_secretkey("pass");
		header->set_subservice("sub");
		
		request->set_algorithmid("bnitems");
		request->set_limit(3000); // soul暂时对本参数不起作用

		request->set_source(atoi(input_data.at(0).c_str())); // 0为app 1为pc
		//request->set_source(0); // 0为app 1为pc
		
		request->set_userid("xuxingjun");
		request->set_cuid("hitskyer");
		request->set_baidu_id("");
		request->set_nm_key("");
		request->set_coor_sys("baidu");
		request->set_x(atof(input_data.at(1).c_str()));
		//request->set_x(13430985.75);
		request->set_y(atof(input_data.at(2).c_str()));
		//request->set_y(3233968.82);
		request->set_area_id(atoi(input_data.at(3).c_str()));
		//request->set_area_id(178);
		
		request->set_item_id_format(1); // 0为string可视格式，1为uint64_t格式

		vector<uint64_t> item_ids;
		//cout<<input_data.size()<<endl;
		for(int i=4; i<input_data.size(); i++){
			uint64_t item_id=strtoul(input_data.at(i).c_str(),NULL, 10);
			item_ids.push_back(item_id);
		}
		//item_ids.push_back(9991479517896860251);
		for (std::vector<uint64_t>::const_iterator itr = item_ids.begin();
				itr != item_ids.end(); ++itr){
			request->add_item_ids((&(*itr)), sizeof(uint64_t));
		}
		
		
		lbs::da::openservice::GetBNItemsResponse* response = new lbs::da::openservice::GetBNItemsResponse();

		// 创建RpcController
		sofa::pbrpc::RpcController* cntl = new sofa::pbrpc::RpcController();
		cntl->SetTimeout(3000);

		long start_time = getCurrentTime();
		// 创建回调函数，回调函数可以预绑定一些参数，譬如callbacked
		google::protobuf::Closure* done = sofa::pbrpc::NewClosure(
				&EchoCallback, cntl, request, response, stub, testdata, params, start_time);
		
		pthread_mutex_lock(&mutex);
		total_req_cnt++;
		pthread_mutex_unlock(&mutex);
		// 发起调用，最后一个参数非NULL表示为异步调用
		stub->GetBNItemsByItem(cntl, request, response, done);
	}catch(exception const& e){
		cout<<e.what()<<endl;
	}
}

int main(int argc, char** argv){
	if(pthread_mutex_init(&mutex,NULL)!=0){
		return -1;
	}
	
	string filepath;
	string ip_port;
	if(argc != 6){
		cout<<help_info<<endl;
		return -1;
	}else{
		ip_port=argv[1];
		filepath=argv[5];
	}
	vector<string> testdata=get_testdata(filepath);
	
	/*vector<string>::iterator it;
	for(it=testdata.begin(); it!=testdata.end(); it++){
		cout<<*it<<endl;
	}*/
	
	//SOFA_PBRPC_SET_LOG_LEVEL(NOTICE);

	// 定义RpcClient
	sofa::pbrpc::RpcClientOptions client_options;
	sofa::pbrpc::RpcClient *rpc_client = new sofa::pbrpc::RpcClient(client_options);

	// 定义RpcChannel
	sofa::pbrpc::RpcChannel *rpc_channel = new sofa::pbrpc::RpcChannel(rpc_client, ip_port);

	// 定义EchoServer服务的桩对象
	lbs::da::openservice::ItemService_Stub *stub = new lbs::da::openservice::ItemService_Stub(rpc_channel);

	/*// 创建Request和Response消息
	sofa::pbrpc::test::EchoRequest* request = new sofa::pbrpc::test::EchoRequest();
	request->set_message("Hello from qinzuoyan01");
	sofa::pbrpc::test::EchoResponse* response = new sofa::pbrpc::test::EchoResponse();

	// 创建RpcController
	sofa::pbrpc::RpcController* cntl = new sofa::pbrpc::RpcController();
	cntl->SetTimeout(3000);

	// 创建回调函数，回调函数可以预绑定一些参数，譬如callbacked
	bool callbacked = false;
	google::protobuf::Closure* done = sofa::pbrpc::NewClosure(
			&EchoCallback, cntl, request, response, &callbacked);

	// 发起调用，最后一个参数非NULL表示为异步调用
	stub.Echo(cntl, request, response, done);*/
	int client_num;
	int testtime;
	
	if(argc != 6){
		cout<<help_info<<endl;
		return -1;
	}else{
		client_num=atoi(argv[2]);
		testtime=atoi(argv[3]);
		stdout_flag=atoi(argv[4]);
	}
	int start = getCurrentTime();
	//begin_time = getCurrentTime();
	int test_time = testtime*60*1000;
	int end = start + test_time;
	
	bool params = false;
	for(int i=0; i<client_num; i++){
		EchoAsynCall(stub, testdata, &params);
	}
	//EchoAsynCall(stub, testdata, &params);
	
	// 等待回调完成，此处为简单的变量探测方式，不推荐
	/*while (!params) {
		usleep(100000);
	}*/
	int current = getCurrentTime();
	while(current < end){
		usleep(1000000);
		current = getCurrentTime();
	}
	
	//rpc_client->Shutdown(); // should call Shutdown here!
	//避免出core，屏蔽
	/*delete stub;
	delete rpc_channel;
	delete rpc_client;*/
	
	//cout<<"\033[11B";
	/*cout<<"test_time: "<<testtime<<" min"<<endl;
	cout<<"total_req_cnt: "<<total_req_cnt<<endl;
	cout<<"total_res_cnt: "<<total_res_cnt<<endl;
	cout<<"total_nul_cnt: "<<total_nul_cnt<<endl;
	cout<<"total_err_cnt: "<<total_err_cnt<<endl;
	cout<<"QPS(query per second): "<<((double)total_res_cnt/(double)test_time)*1000<<endl<<endl;
	
	cout<<"avg_res_time: "<<(double)total_res_time/(double)total_res_cnt<<" ms"<<endl;
	cout<<"below_10(ms): "<<below_10<<endl;
	cout<<"between_10_20(ms): "<<between_10_20<<endl;
	cout<<"between_20_30(ms): "<<between_20_30<<endl;
	cout<<"over_30(ms): "<<over_30<<endl;*/
	return 0;
}

