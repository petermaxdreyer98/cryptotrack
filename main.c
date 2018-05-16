#include <stdio.h>
#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define CRYPTOCOMPARE_SINGLE_PRICE "https://min-api.cryptocompare.com/data/price"
#define CRYPTOCOMPARE_MULTI_PRICE "https://min-api.cryptocompare.com/data/pricemulti"
#define CRYPTOCOMPARE_PRICE_HISTORICAL "https://min-api.cryptocompare.com/data/histoday?fsym=%s&tsym=%s&limit=%d&toTs=%d"
// %s in the following define is a Coinbase-style currency pairing (i.e. "ETH-USD")
// The date is specified as YYYY-MM-DD UTC
#define COINBASE_PRICE_ENDPOINT "https://api.coinbase.com/v2/prices/%s/spot?date=%s"
// %s in the following define is a Binance-style currency pairing (i.e. ETHBTC)
#define BINANCE_PRICE_ENDPOINT "https://api.binance.com/v1/ticker/price/symbol=%s"

#define COLOR_RED "\033[31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_RESET "\033[37;0m"

struct ResponseData {
				char* htmlData;
};

struct CLIArgs{
				short flags;
				short currencyCount;
				char** currencies;
};

float getCurrencyPrices(char* tickers[], short tickerCount);
size_t handleRemoteResponse(char *ptr, size_t size, size_t nmemb, void* userdata);
char* getJSONLevel(char* jsonString, char* key);
char* cleanupJSONTier(char* tier);
char* retrieveCurrentPrice(char* data, char* ticker);
struct CLIArgs* doArgs(int argc, char** argv);
float retrieveHistoricalPrice(CURL* curl, char* ticker, time_t date);
short countAllStringVars(char* string);
char* cleanupJSONEntry(char* tier);

int main(int argc, char** argv){
				struct CLIArgs* cliArgs = doArgs(argc, argv);
				if(cliArgs->currencyCount == 0){
								// Insert some default currencies to be processed.
								cliArgs->currencies = (char**)malloc(sizeof(char*)*4);
								cliArgs->currencies[0] = "BTC";
								cliArgs->currencies[1] = "LTC";
								cliArgs->currencies[2] = "ETH";
								cliArgs->currencies[3] = "XMR";
								cliArgs->currencyCount = 4;
				}
				getCurrencyPrices(cliArgs->currencies, cliArgs->currencyCount);
				return 0;
}

float getCurrencyPrices(char* tickers[], short tickerCount){
				CURL* curl;
				CURLcode res;

				curl = curl_easy_init();
				if(!curl){
								printf("Curl could not be initialized!\n");
								return 1;
				}
				struct ResponseData responseData;
				int stringLen = 0;
				for(int c=0;c!=tickerCount;c++){
								char* currentTicker = tickers[c];
								stringLen += strlen(currentTicker);
				}
				char* currencyList = (char*)malloc(stringLen + tickerCount);
				int listPosition = 0;
				for(int c=0;c!=tickerCount;c++){
								memcpy(currencyList + listPosition, tickers[c], strlen(tickers[c]));
								currencyList[strlen(tickers[c])+listPosition] = ',';
								listPosition += strlen(tickers[c])+1;
				}
				currencyList[stringLen + tickerCount] = 0;
				// Now generate the request URL to find the current price for libcurl.
				int requestURLLength = strlen(CRYPTOCOMPARE_MULTI_PRICE) + strlen("?fsyms=") + strlen(currencyList) + strlen("&tsyms=USD\0");
				char* requestURL = (char*)malloc(sizeof(char) * requestURLLength);
				sprintf(requestURL, "%s?fsyms=%s&tsyms=USD\0", CRYPTOCOMPARE_MULTI_PRICE, currencyList);
				curl_easy_setopt(curl, CURLOPT_URL, requestURL);
				curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);
				curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, handleRemoteResponse);
				res = curl_easy_perform(curl);
				// We now have the response, parse it.
				char* currentPrice;
				for(int c=0;c!=tickerCount;c++){
								currentPrice = retrieveCurrentPrice(responseData.htmlData, tickers[c]);
								// Now attempt to get the prices from an hour ago.
								// It appears that cryptocompare does not allow batch historical data
								// retrieval so this is inefficient as it gets.
								// Calculate the total length of the request string.
								int thisRequestStringLength = strlen(CRYPTOCOMPARE_PRICE_HISTORICAL); 
								// Replace all instances of %s and %d as they will not be
								// factored in to the final output.
								char* currentStringLocation = CRYPTOCOMPARE_PRICE_HISTORICAL;
								char* stringLocation = currentStringLocation;
								char* decimalLocation = currentStringLocation;
								thisRequestStringLength = strlen(CRYPTOCOMPARE_PRICE_HISTORICAL) + countAllStringVars(CRYPTOCOMPARE_PRICE_HISTORICAL)*sizeof(char)*2;
								thisRequestStringLength += strlen(tickers[c]) + strlen("USD"); + 2;
								char* historicalURL = (char*)malloc(sizeof(char)*thisRequestStringLength);
								// Calculate the UNIX timestamp from an hour ago to retrieve the price.
								time_t currentTime = time(NULL);
								// Calculate the UNIX timestamp from an hour ago to retrieve the price.
								// TODO: Optimize this.
								float oneHourAvg = retrieveHistoricalPrice(curl, tickers[c], currentTime - (60*60));
								float twentyFourHourAvg = retrieveHistoricalPrice(curl, tickers[c], currentTime - (60*60*24));
								float sevenDayAvg = retrieveHistoricalPrice(curl, tickers[c], currentTime  -(60*60*24*7));
								// float twentyFourHourVal = retrieveHistoricalPrice(curl, tickers[c], oneHourAgo);
								// Calculate the percentage difference.
								// TODO: Determine why values for ZCash and ZClassic crash the software.
								float currentPrice = atof(retrieveCurrentPrice(responseData.htmlData, tickers[c]));
								// Now calculate the percentage change for each time period.
								printf("%s: %f ", tickers[c], currentPrice, (1-(oneHourAvg/currentPrice))*100, (1-(twentyFourHourAvg/currentPrice))*100, (1-(sevenDayAvg/currentPrice))*100);
								float oneHourPercentage = (1-(oneHourAvg/currentPrice))*100;
								float twentyFourHourPercentage = (1-(twentyFourHourAvg/currentPrice))*100;
								float sevenDayPercentage = (1-(sevenDayAvg/currentPrice))*100;
								if(oneHourPercentage < 0){fputs(COLOR_RED, stdout);}else{
												fputs(COLOR_GREEN, stdout);}
								printf("%f%s ", oneHourPercentage, COLOR_RESET);
								if(twentyFourHourPercentage < 0){fputs(COLOR_RED, stdout);}else{
												fputs(COLOR_GREEN, stdout);}
								printf("%f%s ", twentyFourHourPercentage, COLOR_RESET);
								if(sevenDayAvg < 0){fputs(COLOR_RED, stdout);}else{
												fputs(COLOR_GREEN, stdout);}
								printf("%f%s\n", sevenDayPercentage, COLOR_RESET);
				}
}

size_t handleRemoteResponse(char *ptr, size_t size, size_t nmemb, void* userdata){
				struct ResponseData* thisResponse = (struct ResponseData*)userdata;
				thisResponse->htmlData = (char*)malloc(size*nmemb) + sizeof(char);
				memcpy(thisResponse->htmlData, ptr, size*nmemb);
				thisResponse->htmlData[size*nmemb] = 0;
				return size*nmemb;
}

// The following is a barely function pseudo-tokenizer monstrosity that is here
// for the sole purpose of avoiding linking a full JSON parsing library.
// It works just well enough to get the job done.  I don't expect any awards.
char* getJSONLevel(char* jsonString, char* key){
				// Scan through the provided JSON string until the proper key is found.
				char* keyLocation = strstr(jsonString, key);
				// We now have the key location, locate the end of it and return this
				// value.
				char* needle = (char*)malloc(sizeof(char)*2);
				needle = ":\0";
				char* returnVal = strstr(keyLocation, needle);
				return returnVal;
}

// Truncate at the first occurence of '}'
char* cleanupJSONTier(char* tier){
				tier += 1;
				char* breakPos = strstr(tier, "}");
				char* returnString = (char*)malloc(sizeof(char)*(breakPos-tier));
				memcpy(returnString, tier, (int)(breakPos-tier));
				returnString[breakPos-tier] = 0;
				return returnString;
};

// TODO: Optimize this function.
char* cleanupJSONEntry(char* tier){
				char* breakPos = strstr(tier, ",");
				char* returnString = (char*)malloc(sizeof(char)*(breakPos-tier)+1);
				memcpy(returnString,tier,breakPos-tier);
				returnString[(breakPos-tier)] = 0;
				returnString = &returnString[1];
				return returnString;
}

char* retrieveCurrentPrice(char* data, char* ticker){
				char* thisStart = getJSONLevel(data, ticker);				
				char* usd = getJSONLevel(thisStart, "USD");
				usd = cleanupJSONTier(usd);
				return usd;
}

struct CLIArgs* doArgs(int argc, char** argv){
				// Parse the flags given to the software on the commandline.
				struct CLIArgs* returnArgs = (struct CLIArgs*)malloc(sizeof(struct CLIArgs));
				if(argc == 1){ // No arguments were specified.
								return returnArgs;
				}
				// Arguments have been specified.  Process them.
				for(int c=1;c!=argc;c++){
								char* needle = (char*)malloc(sizeof(char)*2);
								needle = "-f";
								if(strstr(argv[c], needle)){
												// The currency flag has been specified.  Parse out the
												// list and assign it to the struct.
												char* listStart = argv[c+1];
												// We should now be at the list start.
												// Determine the number of currencies in the list based
												// on the number of commas within the string.
												// Generate a 2-dimensional array containing all of the
												// specified currencies.
												short numCurrencies = 0;
												for(short x=0;x!=strlen(listStart);x++){
																if(listStart[x] == ','){
																				numCurrencies++;
																}
												}
												numCurrencies++;
												returnArgs->currencyCount = numCurrencies;
												// We now have the total number of currencies, allocate
												// space for them.
												returnArgs->currencies = (char**)malloc(sizeof(char*)*numCurrencies);
												// The loop for each character in the list.
												short rawTickerLen = 0;
												short currentCount = 0;
												char* tickerPtr = listStart;
												for(short lc=0;lc!=strlen(listStart)+1;lc++){
																// Keep counting until either EOL or the first comma.
																if(listStart[lc] != ',' && listStart[lc] != 0){
																				rawTickerLen++;
																}else{
																				// Determine the number of spaces.
																				short numNonSpaces = 0;
																				for(short v=0;v!=rawTickerLen;v++){
																								if(tickerPtr[v] != ' '){
																												numNonSpaces++;
																								}
																				}
																				char* thisString = (char*)malloc(sizeof(char)*numNonSpaces);
																				short currentPos = 0;
																				for(short v=0;v!=rawTickerLen;v++){
																								if(tickerPtr[v] != ' '){
																												thisString[currentPos] = tickerPtr[v];
																												currentPos++;
																								}
																				}
																				// Convert the letters to uppercase for
																				// particularly picky APIs
																				for(short v=0;v!=numNonSpaces;v++){
																								thisString[v] = toupper(thisString[v]);
																				}	
																				tickerPtr = &listStart[lc]+1;
																				rawTickerLen = 0;
																				returnArgs->currencies[currentCount] = thisString;
																				currentCount++;
																}
												}
								}
				}
				return returnArgs;
}

// TODO: Optimize this
float retrieveHistoricalPrice(CURL* curl, char* ticker, time_t date){
				// Calculate the total length of the request string.
				int thisRequestStringLength = strlen(CRYPTOCOMPARE_PRICE_HISTORICAL); 
				// Replace all instances of %s and %d as they will not be
				// factored in to the final output.
				char* currentStringLocation = CRYPTOCOMPARE_PRICE_HISTORICAL;
				char* stringLocation = currentStringLocation;
				char* decimalLocation = currentStringLocation;
				thisRequestStringLength = strlen(CRYPTOCOMPARE_PRICE_HISTORICAL) + countAllStringVars(CRYPTOCOMPARE_PRICE_HISTORICAL)*sizeof(char)*2;
				thisRequestStringLength += strlen(ticker) + strlen("USD"); + 2;
				char* historicalURL = (char*)malloc(sizeof(char)*thisRequestStringLength);
				// Make a request to the remote server for the spot price for the given
				// currencies at the given date.
				sprintf(historicalURL, CRYPTOCOMPARE_PRICE_HISTORICAL, ticker, "USD", 1, date);
				historicalURL[thisRequestStringLength] = '\0';
				// Calculate the UNIX timestamp from an hour ago to retrieve the price.
				struct ResponseData* thisResponse = (struct ResponseData*)malloc(sizeof(struct ResponseData));
				curl_easy_setopt(curl, CURLOPT_WRITEDATA, thisResponse);
				curl_easy_setopt(curl, CURLOPT_URL, historicalURL);	
				curl_easy_perform(curl);
				// We've retrieve the JSON response from the server.  Now pull
				// out the proper data from it.
				// TODO: Determine a better method.
				// Retrieve the high and the low values and calculate the
				// average.
				char* lowVal = getJSONLevel(thisResponse->htmlData, "close");
				lowVal = cleanupJSONEntry(lowVal);
				// Assume that only the first two decimals are valid.
				// TODO: Support at least four decimals.
				return atof(lowVal); 
}

short countAllStringVars(char* string){
				short numFoundVariables = 0;
				// Loop through all characters in the string.
				for(short c=0;c!=strlen(string);c++){
								if(string[c] == '%'){ // Potential match.
												switch(string[c+1]){
																case 'd':
																case 's':
																				numFoundVariables++;
																				break;
												}	
								}
				}
				return numFoundVariables;
}
