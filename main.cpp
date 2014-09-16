#include <iostream>
#include <string>
#include <fstream>
#include <math.h>
#include <algorithm>
#include "lodepng.h"
#include "lodepng.cpp"
#include <bitset>

using namespace std;

void encode(const char* filename, std::vector<unsigned char>& image, unsigned width, unsigned height)
{
  //Encode the image
  unsigned error = lodepng::encode(filename, image, width, height);

  //if there's an error, display it
  if(error) std::cout << "encoder error " << error << ": "<< lodepng_error_text(error) << std::endl;
}

vector<unsigned char> decode(const char* filename)
{
  std::vector<unsigned char> image; //the raw pixels (RGBA)
  unsigned width, height;

  //decode
  unsigned error = lodepng::decode(image, width, height, filename);

  //if there's an error, display it
  if(error) std::cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;

  return image;
}

//from stackoverflow
string argGet(char ** begin, char ** end, const std::string & option)
{
    char ** itr = std::find(begin, end, "-"+option);
    if (itr != end && ++itr != end)
        return *itr;
    itr = std::find(begin, end, "--"+option);
    if (itr != end && ++itr != end)
        return *itr;
    itr = std::find(begin, end, "/"+option);
    if (itr != end && ++itr != end)
        return *itr;
    return "";
}

bool argExist(char** begin, char** end, const std::string& option)
{
    return std::find(begin, end, "-"+option) != end || std::find(begin, end, "--"+option) != end || std::find(begin, end, "/"+option) != end;
}

void printHelp()
{
    cout << "Usage: SnowCrash [OPTIONS]" << endl
         << "SnowCrash will convert files into PNGs and back" << endl << endl
         << "  -e [FILENAME]\t\tEncode file to PNG" << endl
         << "  -d [FILENAME]\t\tDecode file" << endl
         << "  -f [FILENAME]\t\tSet output filename" << endl
         << "  -w [WIDTH]\t\tSet width PNG (invalid with -r or -d)" << endl
         << "  -r\t\t\tUse recommended width" << endl << endl;
    exit(0);
}

string ToBase256(unsigned long long num)
{
    string conv;
    while(num > 0)
    {
        conv += (unsigned char)(num % 256);
        num /= 256;
    }
    while(conv.length() < 8)
        conv += (unsigned char)num;
    return string(conv.rbegin(), conv.rend());
}

int main(int argc, char* argv[])
{

    unsigned long long width = 0, height = 0, setSize = 0;
    unsigned int pos = string::npos, lastPos = string::npos; //find last dot in filename
    string input, output;
    bool mode = false; //false = decode, true = encode
    bool modeR = false; //true = auto set width to recommended
    bool fSet = false;

    if(argc == 1)
        printHelp();
    if(argc == 2)
    {
        input = argv[1]; //input = filename
        do
        {
            lastPos = pos;
            pos = input.find('.',pos+1);
        }
        while(pos != string::npos);
        pos = lastPos;

        if(input.substr(pos,4) != ".png")
        {
            mode = true;
            modeR = true;
            output = input.substr(0,pos) + ".png";
        }
        else
            output = input.substr(0,pos); //for old version
    }

    if(argExist(argv, argv+argc, "h"))
    {
        printHelp(); //print and exit
    }
    if(argExist(argv,argv+argc,"e")) //encode
    {
        input = string(argGet(argv,argv+argc, "e"));
        mode = true; //true = encode
        output = input + ".png";
    }
    if(argExist(argv,argv+argc,"d")) //decode
    {
        input = string(argGet(argv,argv+argc, "d"));
        //false = decode
        output = input + ".dec";
    }
    if(argExist(argv,argv+argc,"f")) //output filename
    {
        output = argGet(argv,argv+argc, "f");
        fSet = true;
    }

    if(argExist(argv,argv+argc,"w")) //width
    {
        width = atoi(argGet(argv,argv+argc, "w").c_str());
    }
    if(argExist(argv,argv+argc,"r")) //use recommended width
        modeR = true;

    fstream f (input.c_str(), ios::in | ifstream::binary); //binary input stream

    if(mode) //if encode?
    {
        if(f)
        {
            f.seekg(0,f.end);
            unsigned long long length = f.tellg(), rSize = f.tellg();
            f.seekg(0, f.beg);

            char * buffer = new char [length];

            cout << "Reading file \"" << input << "\".." << endl;
            f.read(buffer, length);

            if(f)
                cout << "Read " << f.gcount() << " Bytes." << endl;
            else
            {
                cout << "Error: Only " << f.gcount() << " Bytes were read." << endl;
                exit(1);
            }

            //don't store relative path

            pos = string::npos, lastPos = string::npos; //find last backslash in filename
            do
            {
                lastPos = pos;
                pos = input.find('\\',pos+1);
            }
            while(pos != string::npos);
            pos = lastPos;
            input = input.substr(pos+1);

            pos = string::npos, lastPos = string::npos; //find last slash in filename
            do
            {
                lastPos = pos;
                pos = input.find('/',pos+1);
            }
            while(pos != string::npos);
            pos = lastPos;
            input = input.substr(pos+1);
            length = length + 5 + input.length() + sizeof(rSize); //file length + magic num + \s + filename + version info
            vector<unsigned char> imageO;
            imageO.reserve(length);
            imageO.push_back(255); //magic number ff 02 ff
            imageO.push_back(2);
            imageO.push_back(255);
            imageO.push_back(2); //hardcoded version information
            for(unsigned int i = 0; i < input.length(); ++i)
                imageO.push_back(input[i]); //filename
            imageO.push_back('\0'); //end of filename

            string len = ToBase256(rSize);
            for(int i = 0; i < 8; ++i)
            {
                imageO.push_back(len[i]);
            }

            if(modeR) //if use recommended width
                width = ceil(sqrt(length/4));
            cout << "Recommended width: " << ceil(sqrt(length/4)) << endl;
            cout << "Width : ";
            if(width > 0)
                cout << width << endl;
            else
                cin >> width;
            height = ceil(static_cast<double>(length)/(4*width));
            cout << "Height: " << height << endl;

            for(unsigned long long i = 0; i < length-5-input.length(); ++i)
                imageO.push_back(buffer[i]);
            delete[] buffer;

            if(imageO.size() < width*4*length) //add NUL to end of stream to match image size
                for(unsigned int i = imageO.size(); i < width*4*height; ++i)
                    imageO.push_back(false);

            encode(output.c_str(), imageO, width, height);
        }
        f.close();
    }
    else
    {
        cout << "Decoding \"" << input << "\".." << endl;
        vector<unsigned char> imageI(decode(input.c_str()));
        f.close();
        if(imageI[0] == 255 && imageI[1] == 2 && imageI[2] == 255)
        {
            if(imageI[3] == 2) //encoder v2, no more trailing zeros
            {

                if(!fSet)
                {
                    output = "";
                    for(unsigned int i = 4; imageI[i] != '\0'; ++i)
                        output += imageI[i];
                }

                string op;
                for(unsigned int i = 4; imageI[i] != '\0'; ++i)
                    op += imageI[i];

                string len;
                for(unsigned int i = op.length()+5; i < op.length()+5+8; ++i)
                    len += (unsigned char)imageI[i];
                len = string(len.rbegin(), len.rend());
                for(unsigned int i = 0; i < 8; ++i)
                    setSize += (unsigned long long)len[i]*(unsigned long long)pow(256,i);

                cout << setSize << endl;

                f.open(output.c_str(), ios::out | ofstream::binary);
                cout << "Writing to \"" << output << "\".." << endl;
                f.write((char*)&imageI[output.size() + 5 + 8], setSize);
            }
            else if(imageI[3] == 1) //encoder version 1
            {
                if(!fSet)
                {
                    output = "";
                    for(unsigned int i = 4; imageI[i] != '\0'; ++i)
                        output += imageI[i];
                }
                f.open(output.c_str(), ios::out | ofstream::binary);
                cout << "Writing to \"" << output << "\".." << endl;
                if(setSize == 0) //if setSize wasn't set
                    f.write((char*)&imageI[output.size() + 5], streamsize(imageI.size()) - 5 - output.size());
                else
                    f.write((char*)&imageI[output.size() + 5], setSize);
            }
            else
            {
                cout << "Error: Unknown Version Information." << endl;
                exit(1);
            }

        }
        else //old encoder
        {
            f.open(output.c_str(), ios::out | ofstream::binary);
            cout << "Writing to \"" << output << "\".." << endl;
            if(setSize == 0) //if setSize wasn't set
                f.write((char*)&imageI[0], streamsize(imageI.size()));
            else
                f.write((char*)&imageI[0], setSize);
        }
        f.close();

    }

    return 0;
}
