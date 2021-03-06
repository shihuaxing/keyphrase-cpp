//This code implements the Tf-Idf approach for keyphrase extraction

//Kazi Saidul Hasan, saidul@hlt.utdallas.edu

//09/23/2009

#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cerrno>
#include <vector>
#include <regex.h>
#include <time.h>
#include <math.h>
#include <map>
#include <cctype>

using namespace std;

string fileList="";
string goldKeyList="";
string fileDir="";
string goldKeyDir="";
string outputDir="";

int keyCount=0;

vector<string> files;//document names
vector<string> keylist;//gold keyphrase file names

long docCount=0;
long docLen=0;

map<string,vector<string> > idfCount;//word vs vector of files
vector<string> document;//actual document
vector<string> posTags;//actual document's POS tags

long totalKey=0;
long matched=0;
long predicted=0;

map<string,string> goldKeyMap;//gold keywords' map
vector<string> goldKey;//gold keywords

map<string,double> tfIdf;//word vs its Tf-Idf score
map<string,vector<long> > position;//word vs vector of positions where it appears in the doc

void clear()
{
	document.clear();
	posTags.clear();
	goldKeyMap.clear();
	goldKey.clear();	
	
	tfIdf.clear();
	position.clear();
	
	docLen=0;
}

bool isGoodPOS(string pos)
{
	if(pos=="NN" || pos=="NNS" || pos=="NNP" || pos=="NNPS" || pos=="JJ") return true;
	
	return false;
}

string stringToLower(string strToConvert)
{
	string s="";
	
	for(unsigned int i=0;i<strToConvert.length();i++)
    {
    	s+=tolower((int)strToConvert[i]);
    }
   
    return s;
}

void trim(string& str)  
{  
    size_t startpos=str.find_first_not_of(" \t");  
    size_t endpos=str.find_last_not_of(" \t");  
  
    if((string::npos==startpos) || (string::npos==endpos))	str="";  

    else str=str.substr(startpos,endpos-startpos+1);
}

vector<string> tokenize(const string& str, const string& delimiters)
{
	vector<string> tokens;
    	
    string::size_type lastPos=str.find_first_not_of(delimiters,0);
    	
	string::size_type pos=str.find_first_of(delimiters,lastPos);

	while(string::npos!=pos || string::npos!=lastPos)
	{
		tokens.push_back(str.substr(lastPos,pos-lastPos));
	
		lastPos=str.find_first_not_of(delimiters,pos);
	
		pos=str.find_first_of(delimiters,lastPos);
	}

	return tokens;
}

void readFiles(int version, char* file)
{
	char line[500000];
	size_t len=500000;
	char* token;
	
	if(freopen(file,"r",stdin)==NULL)
	{
	   	cout<<"Failed to open the "<<file<<"  file"<<endl<<"Cause : "<<strerror(errno)<<endl;
		exit(-1);
	}
	
	cin.clear();
	
	while(cin.getline(line,len)!=NULL)
  	{
  		string row=line;
  		
		if(version==1)
		{
			files.push_back(row);
			docCount++;
		}
		else if(version==2) keylist.push_back(row);
 	}
  	
  	fclose(stdin);
}

void readTxtFile(int version, char* file, string paperID)
{
	char line[500000];
	size_t len=500000;
	char* token;
	
	if(freopen(file,"r",stdin)==NULL)
	{
	   	cout<<"Failed to open the "<<file<<"  file"<<endl<<"Cause : "<<strerror(errno)<<endl;
		exit(-1);
	}
	
	cin.clear();
	
	while(cin.getline(line,len)!=NULL)
  	{
  		vector<string> row;
  		
  		token=strtok(line," \t\r\n");
  		
		while(token!=NULL)
		{
			row.push_back(token);
			
			token=strtok(NULL," \t\r\n");
		}
		
		if(row.size()>0)
		{			
			for(int i=0;i<row.size();i++)
			{				
				int index=-1;
				
				//each token from the text file is assumed to have the form "word_PoS"
				index=row[i].find_last_of('_');
				
				string word=row[i].substr(0,index);
				string pos=row[i].substr(index+1);
				
				trim(word);
				
				word=stringToLower(word);
				
				if(word.size()>=2 && (word[0]=='<' || word[word.size()-1]=='>')) continue;
				
				if(version==1)
				{					
					//store the original doc
					document.push_back(word);
					posTags.push_back(pos);
					
					//find POS and filter everything except nouns and adjectives
					if(!isGoodPOS(pos))
					{
						docLen++;
						continue;
					}
									
					docLen++;
					
					position[word].push_back(docLen);
				}
				
				else if(version==2)
				{
					int f=0;
					
					for(int j=0;j<idfCount[word].size();j++)
					{
						if(idfCount[word][j]==paperID) {f=1;break;}	
					}
					
					if(!f) idfCount[word].push_back(paperID);
				}
			}
		}		
  	}
	
  	fclose(stdin);
}

void computeIdfCounts()
{	
	for(int i=0;i<files.size();i++)
	{
		string txt=fileDir+files[i];    	 

		readTxtFile(2,(char *)txt.c_str(),files[i]);
	}
}

void registerGoldKey(string gkey, int paperID)
{
	trim(gkey);
	gkey=stringToLower(gkey);
	
	if(goldKeyMap[gkey]!=gkey)
	{		
		goldKey.push_back(gkey);
		goldKeyMap[gkey]=gkey;
		
		totalKey++;
	}
}

void readGoldKey(char* file, int& i)
{
	char line[50000];
	size_t len=50000;
	char* token;
	
	if(freopen(file,"r",stdin)==NULL)
	{
	   	cout<<"Failed to open the "<<file<<"  file"<<endl<<"Cause : "<<strerror(errno)<<endl;
		exit(-1);
	}
	
	cin.clear();
	
	while(cin.getline(line,len)!=NULL)
	{
		string row=line;
		
		registerGoldKey(row,i);			
	}  		
  	
  	fclose(stdin);
}

double getTfIdfScore(string term)
{
	double tf=(1.0*position[term].size());
	double idf=log((1.0*docCount)/(1.0+idfCount[term].size()))/log(2);
	
	return tf*idf;	
}

void computeTfIdf()
{
	map<string, vector<long> >::iterator ptr;
    
    for(ptr=position.begin();ptr!=position.end();ptr++)
    {
		string word=(*ptr).first;
    	
		if(word!="") tfIdf[word]=getTfIdfScore(word);
    }
}

void replaceLowest(vector<string>& topKey, vector<double>& topKeyVal, string key, double val)
{
	int index=0;
	double minVal=0;
	
	for(int i=0;i<topKeyVal.size();i++)
	{
		if(!i) {index=0; minVal=topKeyVal[i];}
		
		else if(topKeyVal[i]<minVal)
		{
			index=i;
			minVal=topKeyVal[i];
		}
	}
	
	if(val>minVal)
	{
		topKey[index]=key;
		topKeyVal[index]=val;
	}
}

bool isGoldKey(string key)
{		
	for(int i=0;i<goldKey.size();i++)
	{		
		if(goldKey[i]==key)
		{
			return true;
		}
	}
	
	return false;
}

int extractPatterns(map<string,int>& candClause)
{
	long count=0;
	
	vector<string> pattern;
	vector<string> patternPOS;
	
	//selecting candidate patterns
	for(long i=0;i<posTags.size();i++)
	{		
		if(isGoodPOS(posTags[i]))
		{
			pattern.push_back(document[i]);
			patternPOS.push_back(posTags[i]);
		}
		
		else if(pattern.size() && !isGoodPOS(posTags[i]))
		{
			string s="";
			
			for(int j=0;j<pattern.size();j++)
			{
				s+=pattern[j];
				if(j+1<pattern.size()) s+=" ";  
			}
			
			if(patternPOS[patternPOS.size()-1]!="JJ")
			{
				if(!candClause[s]) count++;
				
				candClause[s]++;
			}
			
			pattern.clear();
			patternPOS.clear();
		}
	}
	
	if(pattern.size())
	{
		string s="";
			
		for(int j=0;j<pattern.size();j++)
		{
			s+=pattern[j];
			if(j+1<pattern.size()) s+=" ";  
		}
		
		if(patternPOS[patternPOS.size()-1]!="JJ")
		{
			if(!candClause[s]) count++;
			
			candClause[s]++;
		}
		
		pattern.clear();
		patternPOS.clear();
	}
	
	return count;
}

double getTotalScore(string s)
{
	vector<string> tokens=tokenize(s," ");
	
	double d=0.0;
	
	for(int i=0;i<tokens.size();i++)
	{
		d+=tfIdf[tokens[i]];
	}
	
	return d;
}

void score(char* file)
{
	if(freopen(file,"w",stdout)==NULL)
	{
	   	cout<<"Failed to open the "<<file<<"  file"<<endl<<"Cause : "<<strerror(errno)<<endl;
		exit(-1);
	}
	
	map<string,int> candClause;//clause vs count
	
	vector<string> topKey;
	vector<double> topKeyVal;
	
	extractPatterns(candClause);
	
	map<string,int>::iterator ptr;
		
	for(ptr=candClause.begin();ptr!=candClause.end();ptr++)
	{
		if((*ptr).second)
		{
			string s=(*ptr).first;
			
			double score=getTotalScore(s);
			
			if(topKey.size()<keyCount)
			{
				topKey.push_back(s);
				topKeyVal.push_back(score);				 
			}
			
			else replaceLowest(topKey,topKeyVal,s,score);
		}
	}
	
	predicted+=topKey.size();
	
	for(int i=0;i<topKey.size();i++)
	{
		string pkey=topKey[i];
		
		cout<<pkey<<endl;
		
		//adding some 'ad-hoc' stemming  
		if(isGoldKey(pkey) || isGoldKey(pkey+"s") || 
		   (pkey[pkey.size()-1]=='s' && isGoldKey(pkey.substr(0,pkey.size()-1))))
		{
			matched++;
		}
	}

	fclose(stdout);
	
	freopen("/dev/tty","w",stdout);//redirecting output back to console
}

void readParams(char* file)
{
	char line[50000];
	size_t len=50000;
	char* token;
	
	int lineNo=0;
	
	if(freopen(file,"r",stdin)==NULL)
	{
	   	cout<<"Failed to open the "<<file<<"  file"<<endl<<"Cause : "<<strerror(errno)<<endl;
		exit(-1);
	}
	
	cin.clear();
	
	while(cin.getline(line,len)!=NULL)
  	{
  		string row=line;
  		lineNo++;
  		
  		vector<string> tokens=tokenize(row,"=\t\r\n");
			
		if(tokens.size()==2)
		{
			if(lineNo==1) fileList=tokens[1];
			else if(lineNo==2) goldKeyList=tokens[1];
			else if(lineNo==3) fileDir=tokens[1];
			else if(lineNo==4) goldKeyDir=tokens[1];
			else if(lineNo==5) outputDir=tokens[1];
			else if(lineNo==6) keyCount=atoi(tokens[1].c_str());
		}
 	}
  	
  	fclose(stdin);
}

int main(int argc, char* argv[])
{
	cout<<"Reading params ..."<<endl;
	readParams(argv[1]);
	
	readFiles(1,(char*)fileList.c_str());//document list
    if(goldKeyList!="") readFiles(2,(char*)goldKeyList.c_str());//gold key file list
	
    cout<<"Computing inverse document frequencies ..."<<endl;
    computeIdfCounts();
        
    for(int i=0;i<docCount;i++)
    {    	
    	clear();
    	
		string txt="";
		txt=fileDir+files[i];
	
		cout<<"Processing "<<files[i]<<" ..."<<endl;
		
		readTxtFile(1,(char *)txt.c_str(),files[i]);
		
		string key="";
		
		if(goldKeyDir!="" && i<keylist.size())
		{
			key=goldKeyDir+keylist[i];	    
			readGoldKey((char *)key.c_str(),i);
		}
		
	    computeTfIdf();
	    
		//score each candidate phrase and output top-scoring phrases
    	score((char*)(outputDir+files[i]+".phrases").c_str());
    }

    cout<<"-------------------------------------------------"<<endl;
  	
	double p=(100.0*matched)/(1.0*predicted);    
	double r=(100.0*matched)/(1.0*totalKey);
	double f=(2.0*p*r)/(p+r);
	
	if(p && r && f)
	{	
		cout<<"Recall = "<<r<<endl;
		cout<<"Precision = "<<p<<endl;
		cout<<"F-score = "<<f<<endl;
	}
	
	cout<<"done"<<endl;
}
