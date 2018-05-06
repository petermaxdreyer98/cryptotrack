#include <stdio.h>
#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>

#define CRYPTOCOMPARE_SINGLE_PRICE "https://min-api.cryptocompare.com/data/price"
#define CRYPTOCOMPARE_MULTI_PRICE "https://min-api.cryptocompare.com/data/pricemulti"


struct ResponseData {
				char* htmlData;
};

float getCurrencyPrice(char* tickers[], short tickerCount);
size_t handleRemoteResponse(char *ptr, size_t size, size_t nmemb, void* userdata);

int main(int argc, char** argv){
				char* tickers[] = {"ETH", "NANO", "LTC", "BTC"};
				float ethPrice = getCurrencyPrice(tickers, 4);
				return 0;
}

float getCurrencyPrice(char* tickers[], short tickerCount){
				CURL* curl;
				CURLcode res;

				curl = curl_easy_init();
				if(!curl){
								printf("Curl could not be initialized!\n");
								return 1;
				}
				struct ResponseData responseData;
				if(tickerCount == 1){
								curl_easy_setopt(curl, CURLOPT_URL, "https://min-api.cryptocompare.com/data/price?fsym=ETH&tsyms=USD");
				}else{
								// Calculate the total length of the ticker string.
								int stringLen = 0;
								for(int c=0;c!=tickerCount;c++){
												char* currentTicker = tickers[c];
												stringLen += strlen(currentTicker);
								}
								char* currencyList = (char*)malloc(stringLen + tickerCount);
								int listPosition = 0;
								for(int c=0;c!=tickerCount;c++){
												printf("%d\n", strlen(tickers[c]));
												memcpy(currencyList + listPosition, tickers[c], strlen(tickers[c]));
												currencyList[strlen(tickers[c])+listPosition] = ',';
												listPosition += strlen(tickers[c])+1;
								}
								currencyList[stringLen + tickerCount] = 0;
								printf("%s", currencyList);
								// Now generate the request URL for libcurl.
								int requestURLLength = strlen(CRYPTOCOMPARE_MULTI_PRICE) + strlen('?fsyms=');
								char* requestURL;
								curl_easy_setopt(curl, CURLOPT_URL, "https://min-api.cryptocompare.com/data/pricemulti?fsyms=ETH,NANO&tsyms=USD");
				}
				curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);
				curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, handleRemoteResponse);
				res = curl_easy_perform(curl);
				printf("%s", responseData.htmlData);
}


size_t handleRemoteResponse(char *ptr, size_t size, size_t nmemb, void* userdata){
				struct ResponseData* thisResponse = (struct ResponseData*)userdata;
				thisResponse->htmlData = (char*)malloc(size*nmemb) + sizeof(char);
				memcpy(thisResponse->htmlData, ptr, size*nmemb);
				thisResponse->htmlData[size*nmemb] = 0;
				return size*nmemb;
}
