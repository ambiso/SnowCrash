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
         << "  -w [WIDTH]\t\tSet width PNG" << endl << endl;
    //cin.ignore();
}

int main(int argc, char* argv[])
{

    unsigned long long width = 0, height = 0;

    if(argc > 1)
    {
        string input, output;

        bool mode = false;
        bool w_set = false;

        if(argExist(argv, argv+argc, "h"))
        {
            printHelp();
            return 0;
        }
        if(argExist(argv,argv+argc,"e"))
        {
            input = string(argGet(argv,argv+argc, "e"));
            mode = true; //true = encode
            output = input + ".png";
        }
        if(argExist(argv,argv+argc,"d"))
        {
            input = string(argGet(argv,argv+argc, "d"));
            //false = decode
            output = input + ".dec";
        }
        if(argExist(argv,argv+argc,"f"))
        {
            output = argGet(argv,argv+argc, "f");
        }

        if(argExist(argv,argv+argc,"w"))
        {
            string buffer = argGet(argv,argv+argc, "w");
            width = atoi(buffer.c_str());
            w_set = true;
        }

        fstream f (input.c_str(), ios::in | ifstream::binary);

        if(mode)
        {
            if(f)
            {
                f.seekg(0,f.end);
                unsigned long long length = f.tellg();
                f.seekg(0, f.beg);

                char * buffer = new char [length];

                cout << "Reading file \"" << input << "\".." << endl;
                f.read(buffer, length);
                delete[] buffer;

                if(f)
                    cout << "Read " << f.gcount() << " Bytes." << endl;
                else
                    cout << "Error: only " << f.gcount() << " Bytes were read." << endl;

                vector<unsigned char> image_o;
                for(unsigned long long i = 0; i < length; ++i)
                    image_o.push_back(buffer[i]);


                cout << "Recommended width: " << static_cast<long long>(sqrt(length/4)) << endl;
                cout << "Width : ";
                if(w_set)
                    cout << width << endl;
                else
                    cin >> width;
                height = ceil(static_cast<double>(length)/(4*width));
                cout << "Height: " << height << endl;

                if(image_o.size() < width*4*length)
                    for(unsigned int i = image_o.size(); i < width*4*height; ++i)
                        image_o.push_back(false);

                encode(output.c_str(), image_o, width, height);
            }

            f.close();
        }
        else
        {
            cout << "Decoding \"" << input << "\".." << endl;
            vector<unsigned char> image_i(decode(input.c_str()));
            f.close();
            f.open(output.c_str(), ios::out | ofstream::binary);
            cout << "Writing to \"" << output << "\".." << endl;
            if(f)
                f.write((char*)&image_i[0], streamsize(image_i.size()));
            else
                cout << "Error writing file.";
            f.close();
        }
    }
    else
        printHelp();

    return 0;
}
