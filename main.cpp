#include <iostream>
#include <string>
#include <fstream>
#include <math.h>
#include <algorithm>
#include "lodepng.h"
#include "lodepng.cpp"

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
         << "  -r\t\t\tUse recommended width" << endl
         << "  -s [SIZE]\t\tWill only decode [SIZE] number of Bytes." << endl << endl;
    //cin.ignore();
}

int main(int argc, char* argv[])
{

    unsigned long long width = 0, height = 0, setSize = 0;

    if(argc > 1)
    {
        string input, output;

        bool mode = false; //false = decode, true = encode
        bool modeR = false; //true = auto set width to recommended

        if(argExist(argv, argv+argc, "h"))
        {
            printHelp();
            return 0;
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
            output = argGet(argv,argv+argc, "f");

        if(argExist(argv,argv+argc,"w")) //width
        {
            width = atoi(argGet(argv,argv+argc, "w").c_str());
        }
        if(argExist(argv,argv+argc,"r")) //use recommended width
            modeR = true;

        if(argExist(argv,argv+argc,"s")) //only write setSize amount of Bytes
            setSize = atoi(argGet(argv,argv+argc, "s").c_str());

        fstream f (input.c_str(), ios::in | ifstream::binary); //binary input stream

        if(mode) //if encode?
        {
            if(f)
            {
                f.seekg(0,f.end);
                unsigned long long length = f.tellg();
                f.seekg(0, f.beg);

                char * buffer = new char [length];

                cout << "Reading file \"" << input << "\".." << endl;
                f.read(buffer, length);

                if(f)
                    cout << "Read " << f.gcount() << " Bytes." << endl;
                else
                    cout << "Error: Only " << f.gcount() << " Bytes were read." << endl;

                vector<unsigned char> imageO;
                imageO.reserve(f.gcount());
                for(unsigned long long i = 0; i < length; ++i)
                    imageO.push_back(buffer[i]);
                delete[] buffer;

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

            f.open(output.c_str(), ios::out | ofstream::binary);
            cout << "Writing to \"" << output << "\".." << endl;
            if(setSize == 0) //if setSize wasn't set
                f.write((char*)&imageI[0], streamsize(imageI.size()));
            else
                f.write((char*)&imageI[0], setSize);
            f.close();
        }
    }
    else
        printHelp();

    return 0;
}
