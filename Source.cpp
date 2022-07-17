#include <iostream>
#include <curl/curl.h>
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>

using namespace std;



size_t writeFunction(void* ptr, size_t size, size_t nmemb, string* data) {
	data->append((char*)ptr, size * nmemb);
	return size * nmemb;
}

string clearingUrl(const string& url) {
	string decoded = curl_easy_unescape(NULL, url.c_str(), 0, NULL);
	for (size_t i = 4; i < decoded.size(); ++i)
		if (decoded[i - 4] == 'h' && decoded[i - 3] == 't' && decoded[i - 2] == 't' && decoded[i - 1] == 'p' && decoded[i] == 's') {
			decoded.erase(i, 1);
			break;
		}
	return decoded;
}

string makeUrl(const string& url1, const string& url2) {
	if (url2.find(".php") != -1) return "";
	if (url2[0] == url2[1])
		return url2;
	int t = 0, s = 0;
	for (int i = 0; i < url1.size(); ++i) {
		if (url1[i] == '/') ++t;
		if (t == 3) {
			s = i;
			break;
		}
	}
	return url1.substr(0, s) + url2;
}

bool search(const vector<string>& arr, const string& s) {
	for (const auto& i : arr)
		if (i == s) return false;
	return true;
}

void findRefsOther(const string& page, vector<string>& refs, string url) {
	int start = 0, start2 = 0;
	short int flag = 0, flag2 = 0;
	string ref = "";
	for (size_t i = 6; i < page.size(); ++i) {
		if (flag == 0 && page[i - 2] == '<' && page[i - 1] == 'a' && page[i] == ' ')
			flag = 1;
		else if (flag == 1 && page[i - 3] == '<' && page[i - 2] == '/' && page[i - 1] == 'a' && page[i] == '>')
			flag = 0;
		else if (flag == 1 && page[i - 3] == 'h' && page[i - 2] == 't' && page[i - 1] == 't' && page[i] == 'p') {
			flag = 2;
			start = i - 3;
		}
		else if (flag == 2 && (page[i] == '"' || page[i] == char(39) || page[i] == '&')) {
			ref = curl_easy_unescape(NULL, page.substr(start, i - start).c_str(), 0, NULL);
			if (search(refs, ref))
				refs.push_back(ref);
			start = 0;
			flag = 0;
		}
		if (flag2 == 0 && page[i - 2] == '<' && page[i - 1] == 'a' && page[i] == ' ')
			flag2 = 1;
		else if (flag2 == 1 && page[i - 3] == '<' && page[i - 2] == '/' && page[i - 1] == 'a' && page[i] == '>')
			flag2 = 0;
		else if (flag2 == 1 && page[i - 6] == 'h' && page[i - 5] == 'r' && page[i - 4] == 'e' && page[i - 3] == 'f' && page[i - 2] == '=' && page[i - 1] == '"' && page[i] == '/') {
			flag2 = 2;
			start2 = i;
		}
		else if (flag2 == 2 && (page[i] == '"' || page[i] == char(39) || page[i] == '&')) {
			ref = curl_easy_unescape(NULL, makeUrl(url, page.substr(start2, i - start2)).c_str(), 0, NULL);
			if (search(refs, ref) && !ref.empty())
				refs.push_back(ref);
			start2 = 0;
			flag2 = 0;
		}
	}
}

vector<string> findRefsGoogle(const string& page) {
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
			refs.push_back(ref);
		}
	return refs;
}

void parse_page(const string& url, const string& proxy, string& page) {
	string host = "", name = "", password = "";
	auto curl = curl_easy_init();
	if (!proxy.empty()) {
		size_t tmp = proxy.find("//") + 2;
		size_t tmp2 = proxy.find(':', tmp);
		size_t tmp3 = proxy.find('@') + 1;
		host = proxy.substr(0, tmp) + proxy.substr(tmp3, proxy.size() - tmp3);
		name = proxy.substr(tmp, tmp2 - tmp);
		password = proxy.substr(tmp2 + 1, tmp3 - tmp2 - 2);
	}

	auto clear_url = clearingUrl(url);

	curl_easy_setopt(curl, CURLOPT_URL, clear_url.c_str());
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
	curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 50L);
	curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFunction);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);	// redirect from http to https 

	//				!!!PROXY!!!
	curl_easy_setopt(curl, CURLOPT_PROXY, host.c_str());
	curl_easy_setopt(curl, CURLOPT_PROXYUSERNAME, name.c_str());
	curl_easy_setopt(curl, CURLOPT_PROXYPASSWORD, password.c_str());
	//				!!!PROXY!!!

	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &page);
	curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	curl = NULL;
}

void parser(const string& file_r, const string& file_w, const short int& start, const short int& end, const string& proxy) {
	ifstream file1(file_r);
	string word = "", url = "", page = "";
	vector<string>refs_o, refs_g;
	while (!file1.eof()) {
		getline(file1, word);
		replace(word.begin(), word.end(), ' ', '+');
		for (int num_page = start; num_page < end; num_page += 10) {
			url = "https://www.google.com/search?q=" + word + "&start=" + to_string(num_page);
			parse_page(url, proxy, page);
			refs_g = findRefsGoogle(page);
			for (const auto& ref : refs_g) {
				page = "";
				refs_o.clear();
				parse_page(ref, proxy, page);
				findRefsOther(page, refs_o, ref);
				system("cls");
				cout << page;
				ofstream file2(file_w, ios_base::app);
				file2 << ref << '\n';
				for (const auto& i : refs_o)
					file2 << i << '\n';
				file2.close();
			}
		}
	}
}

void deep(const string& url, vector<string>& refs, const int& lvl, const string& proxy, const int& start) {
	if (lvl > 0) {
		string page;
		parse_page(url, proxy, page);
		findRefsOther(page, refs, url);
		int end = refs.size();
		for (int i = start; i < end; ++i)
			deep(refs[i], refs, lvl - 1, proxy, refs.size() - 1);
	}
	else
		return;
}

int main(int argc, char* argv[]) {
	curl_global_init(CURL_GLOBAL_DEFAULT);
	if (argc == 5) {
		string file_r = *(argv + 1), file_w = *(argv + 2);
		int lvl = stoi(*(argv + 3));
		string proxy = *(argv + 4);
		ifstream file1(file_r);
		string tmp;
		while (!file1.eof()) {
			file1 >> tmp;
			cout << tmp << "\n";
			ofstream file2(file_w, ios_base::app);
			vector<string>refs;
			deep(tmp, refs, lvl, proxy, 0);
			for (string i : refs) file2 << i << endl;
			file2.close();
		}
		file1.close();
	}
	else if (argc == 6) {
		string file_r = *(argv + 1), file_w = *(argv + 2);
		int start = stoi(*(argv + 3)), end = stoi(*(argv + 4));
		string proxy = *(argv + 5);
		parser(file_r, file_w, start, end, proxy);
	}
	curl_global_cleanup();
	return 0;
}