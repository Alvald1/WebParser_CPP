#include <iostream>
#include <curl/curl.h>
#include <string>
#include <vector>

using namespace std;

size_t writeFunction(void* ptr, size_t size, size_t nmemb, string* data) {
	data->append((char*)ptr, size * nmemb);
	return size * nmemb;
}

vector<string> findRefs(string& page) {
	int start = 0;
	vector<string>refs;
	string ref = "";
	short int flag = 0;
	for (size_t i = 6; i < page.size(); ++i)
		if (flag == 0 && page[i - 6] == '/' && page[i - 5] == 'u' && page[i - 4] == 'r' && page[i - 3] == 'l' && page[i - 2] == '?' && page[i - 1] == 'q' && page[i] == '=') {
			flag = 1;
			start = i;
		}
		else if (flag == 1 && page[i] == '&') {
			ref = page.substr(start + 1, i - start - 1);
			flag = 2;
		}
		else if (flag == 2 && page[i - 4] == '"' && page[i - 3] == '>' && page[i - 2] == '<' && page[i - 1] == 'h' && page[i] == '3') {
			start = 0;
			flag = 0;
			string decoded = curl_easy_unescape(NULL, ref.c_str(), 0, NULL);
			refs.push_back(decoded);
		}
	return refs;
}

void parser(string& url, string& proxy_url, string& response_string) {
	string host = "", name = "", password = "";
	auto curl = curl_easy_init();
	if (!proxy_url.empty()) {
		size_t tmp = proxy_url.find("//") + 2;
		size_t tmp2 = proxy_url.find(':', tmp);
		size_t tmp3 = proxy_url.find('@') + 1;
		host = proxy_url.substr(0, tmp) + proxy_url.substr(tmp3, proxy_url.size() - tmp3);
		name = proxy_url.substr(tmp, tmp2 - tmp);
		password = proxy_url.substr(tmp2 + 1, tmp3 - tmp2 - 2);
	}

	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
	curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 50L);
	curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFunction);

	//				!!!PROXY!!!
	curl_easy_setopt(curl, CURLOPT_PROXY, host.c_str());
	curl_easy_setopt(curl, CURLOPT_PROXYUSERNAME, name.c_str());
	curl_easy_setopt(curl, CURLOPT_PROXYPASSWORD, password.c_str());
	//				!!!PROXY!!!

	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
	curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	curl = NULL;
}

int main() {
	curl_global_init(CURL_GLOBAL_DEFAULT);

	string word = "q";
	int num_page = 0;
	string url = "http://www.google.com/search?q=" + word + "&start=" + to_string(num_page);
	string proxy_url = "";

	string page;

	parser(url, proxy_url, page);

	cout << page;
	auto refs = findRefs(page);

	curl_global_cleanup();
	return 0;
}