#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
using namespace std;

//most lines in a wavefront obj file contain either 3 or 4 float values
struct Vector4
{
    float x = 0;
    float y = 0;
    float z = 0;
    float w = 1.0; //in obj format, w defaults to 1.0 if unused
};

//most lines in a wavefront obj file contain either 3 or 4 float values
struct Vector3
{
    float x = 0;
    float y = 0;
    float z = 0;
};

//im honestly not sure the best way to store f data but this will do for starters
struct fIndex
{
    //Vector3 entry0;
    //Vector3 entry1;
    //Vector3 entry2;
    int entry0[3];
    int entry1[3];
    int entry2[3];
};

//in a SOSE mesh file, there are vertex entries containing the corresponding values
struct meshVertex
{
    Vector3 position;
    Vector3 Normal;
    Vector3 Tangent;
    string color = "0"; //color value seems to always be zero
    float U0;
    float V0;
    float U1;
    float V1;
};

//in a SOSE mesh file, there are triangle entries containing the index of stuff from (probably) mesh vertex entries
struct triangleEntry
{
    int indexVert0;
    int indexVert1;
    int indexVert2;
    int indexMat; //always 0 pretty much
};

//Splits a string into seperate strings based on a delimiting character. Blatantly copy-pasted from https://java2blog.com/split-string-space-cpp/
void tokenize(std::string const &str, const char delim, std::vector<std::string> &out);

//takes a line beginning with 'f' from a wavefront obj file and converts it to struct format
fIndex rawDataToFindex(string fLine);

//parses to a mesh entry and outputs to the output stream
void generateMesh(ofstream *file, vector<Vector4> verticies, vector<Vector3> texCoordinates, vector<Vector3> norm, vector<fIndex> data);

//instead of making triangles out of points, make points of of triangles. It's not stupid if it works
void generateMeshBetter(ofstream *file, vector<Vector4> verticies, vector<Vector3> texCoordinates, vector<Vector3> norm, vector<fIndex> data, vector<string> specialPointNames, vector<Vector3> specialPointPositions, vector<Vector3> specialRotations);

void pointToFilestream(ofstream *file, string pointName, Vector3 position, Vector3 rotation);

bool alreadyExists(vector<int> points, int *pointToCheck, int &existPosition);

//returns the largest of 3 ints
int largestInt(int i1, int i2, int i3);

bool isSpecialPoint(string input);

//in blender, objects are always in the format of [name]_[name of vertex array]. This function chops of anything after an underscore
string trimStringToSOSEPoint(string input);

int LOG_LEVEL = 0;

//convert obj file to sins of a solar empire compatible .mesh
//xsi is a bullshit-ass format that should've never existed
int main(int argc, char *argv[])
{
    string inputFilename = "input.obj";
    if (argc > 1)
    {
        //inputFilename = argv[0];
        cout << "filename = " << argv[1] << endl;
        inputFilename = argv[1];
    }
    // note: all 3d stuff I ever make (including this program) ONLY WORKS for obj files exported with blender with all faces converted to tris
    // to do this, go to edit mode, "a" to select all faces. Then click on the "face" button and then "triangulate faces". Note that only triangle faces are valid and quads are not supported (even though quads are usually supported in 3d stuff I make, just not in this one)
    // This should go without saying but there can be ABSOLUTELY NO N-GONS
    vector<Vector4> geoVerts = vector<Vector4>(); //store 3ds geometric coordinates
    vector<Vector3> texCoords = vector<Vector3>(); //store 3ds texture coordinates
    vector<Vector3> vertexNormals = vector<Vector3>(); //vertex normals. should always contain 3 floats if exporting to obj from blender using "the correct method"
    vector<fIndex> faceData = vector<fIndex>();//the f lines in a wavefront obj file are basically tri faces and we need to save this information to generate a valid mesh

    //================================================
    //the .mesh related data
    vector<meshVertex> SOSEVerts = vector<meshVertex>();
    vector<triangleEntry> SOSETris = vector<triangleEntry>();

    vector<string> specialPointNames = vector<string>();
    vector<Vector3> specialPoints = vector<Vector3>();
    vector<Vector3> specialRotation = vector<Vector3>();
    //=========================================

    ifstream inputFile;
    inputFile.open("input.obj");

    string fileLine;
    bool inhibitLoading = false;
    bool gotVertexAlready = false;
    bool gotRotationAlready = false;
    int numSpecialPoints = 0;
    while (getline(inputFile, fileLine))
    {
        //cout << fileLine << endl;
        string firstChars = "";
        firstChars+=fileLine.at(0);
        firstChars+=fileLine.at(1);

        //check for object names
        if (firstChars == "o ")
        {
            if (isSpecialPoint(fileLine.substr(2, fileLine.length())))
            {
                //don't load point object data into the vertex points. There's going to be more than enough crap in there already
                inhibitLoading = true;
                gotVertexAlready = false;
                gotRotationAlready = false;
                string newPointName = trimStringToSOSEPoint(fileLine.substr(2, fileLine.length()));
                specialPointNames.push_back(newPointName);
                cout << "found special point named " << newPointName << endl;
            }
            else
            {
                //if a non-SOSE point object was found, enable 3d data loading again
                inhibitLoading = false;
                gotVertexAlready = false;
                gotRotationAlready = false;
                numSpecialPoints++;
            }
        }
        //get all the geometric verticies
        else if (firstChars == "v ")
        {
            if (inhibitLoading && !gotVertexAlready)
            {
                //if inhibit loading is enabled, we need to store all the points into a different place
                vector<string> fuck = vector<string>();
                tokenize(fileLine, ' ', fuck);
                //cout << "v = " << fuck.at(1) << " " << fuck.at(2) << " " << fuck.at(3) << endl;
                Vector3 newVert;
                newVert.x = stof(fuck.at(1));
                newVert.y = stof(fuck.at(2));
                newVert.z = stof(fuck.at(3));
                specialPoints.push_back(newVert);
                cout << "position of point " << specialPointNames.back() << " = " << newVert.x << ", " << newVert.y << ", " << newVert.z << endl;
                gotVertexAlready = true;
            }
            else
            {
                //if inhibit loading is disabled, we want to store all the points into the mesh
                vector<string> fuck = vector<string>();
                tokenize(fileLine, ' ', fuck);
                //cout << "v = " << fuck.at(1) << " " << fuck.at(2) << " " << fuck.at(3) << endl;
                Vector4 newVert;
                newVert.x = stof(fuck.at(1));
                newVert.y = stof(fuck.at(2));
                newVert.z = stof(fuck.at(3));
                geoVerts.push_back(newVert);
            }
        }
        //get all the texture coordinates
        else if (firstChars == "vt" && !inhibitLoading)
        {
            vector<string> fuck = vector<string>();
            tokenize(fileLine, ' ', fuck);
            //cout << "vt = " << fuck.at(1) << " " << fuck.at(2) << endl;
            Vector3 newUV;
            newUV.x = stof(fuck.at(1));
            newUV.y = stof(fuck.at(2));
            newUV.z = 0;//if you use blender to convert all faces to tris before exporting to obj (as required by this program), this will always be zero
            texCoords.push_back(newUV);
        }
        //get all the vertex normals
        else if (firstChars == "vn")
        {
            if (inhibitLoading && !gotRotationAlready)
            {
                vector<string> fuck = vector<string>();
                tokenize(fileLine, ' ', fuck);

                Vector3 newVert;
                newVert.x = stof(fuck.at(1));
                newVert.y = stof(fuck.at(2));
                newVert.z = stof(fuck.at(3));
                specialRotation.push_back(newVert);
                cout << "rotation of point " << specialPointNames.back() << " = " << newVert.x << ", " << newVert.y << ", " << newVert.z << endl;

                gotRotationAlready = true;
            }
            else
            {
                vector<string> fuck = vector<string>();
                tokenize(fileLine, ' ', fuck);
                //cout << "vn = " << fuck.at(1) << " " << fuck.at(2) << " " << fuck.at(3) << endl;
                Vector3 newN;
                newN.x = stof(fuck.at(1));
                newN.y = stof(fuck.at(2));
                newN.z = 0;//if you use blender to convert all faces to tris before exporting to obj (as required by this program), this will always be zero
                vertexNormals.push_back(newN);
            }
        }
        //now the hard part
        else if (firstChars == "f " && !inhibitLoading)
        {
            //this operation is complex enough to get its own function
            faceData.push_back(rawDataToFindex(fileLine));
        }
    }

    inputFile.close();

    //finished parsing the obj file
    cout << "Finished parsing obj file" << endl;
    cout << "There are " << geoVerts.size() << " verticies, " << texCoords.size() << " texture coordinates and " << vertexNormals.size() << " vertex normals" << endl;
    cout << "There are " << faceData.size() << " obj-defined faces" << endl;

    //run some checks and inform the user of any findings
    if (geoVerts.size() > 1000)
    {
        cout << "Warning: mesh size exceeds 1000 verticies. This is still valid and allowed by the game engine but you typically want your meshes to contain less than 1000 verts for memory and performance reasons" << endl;
    }
    else if (geoVerts.size() > 5000)
    {
        cout << "Error. Mesh has more than 5000 verticies. The original xsi converter rejects models this large. This may not work when you try to load it into the game" << endl;
    }

    //if the right type of log level is enabled, print the parsed contents of the face data
    if (LOG_LEVEL > 0)
    {
        for (int i = 0; i < faceData.size(); i++)
        {
            cout << faceData.at(i).entry0[0] << "/" << faceData.at(i).entry0[1] << "/" << faceData.at(i).entry0[2] << " " << faceData.at(i).entry1[0] << "/" << faceData.at(i).entry1[1] << "/" << faceData.at(i).entry1[2] << " " << faceData.at(i).entry2[0] << "/" << faceData.at(i).entry2[1] << "/" << faceData.at(i).entry2[2] << endl;
        }
    }

    //time to start constructing the mesh file from the data we retrieved
    ofstream outputFile("output.mesh");
    generateMeshBetter(&outputFile, geoVerts, texCoords, vertexNormals, faceData, specialPointNames, specialPoints, specialRotation);

    return 0;
}

//Splits a string into seperate strings based on a delimiting character. Blatantly copy-pasted from https://java2blog.com/split-string-space-cpp/
void tokenize(std::string const &str, const char delim, 
            std::vector<std::string> &out) 
{ 
    // construct a stream from the string 
    std::stringstream ss(str); 
 
    std::string s; 
    while (std::getline(ss, s, delim)) { 
        out.push_back(s); 
    } 
}

//takes a line beginning with 'f' from a wavefront obj file and converts it to struct format
fIndex rawDataToFindex(string fLine)
{
    // split each entry into its own vector
    vector<string> fuck = vector<string>();
    tokenize(fLine, ' ', fuck);

    //if the user actually followed instructions and made their obj only have tri faces, there will be 3 segments to each f line
    vector<string> seg0 = vector<string>();
    vector<string> seg1 = vector<string>();
    vector<string> seg2 = vector<string>();
    tokenize(fuck.at(1), '/', seg0);
    tokenize(fuck.at(2), '/', seg1);
    tokenize(fuck.at(3), '/', seg2);

    //string data has been separated. make it into a struct
    fIndex newEntry;
    newEntry.entry0[0] = stoi(seg0.at(0));
    newEntry.entry0[1] = stoi(seg0.at(1));
    newEntry.entry0[2] = stoi(seg0.at(2));

    newEntry.entry1[0] = stoi(seg1.at(0));
    newEntry.entry1[1] = stoi(seg1.at(1));
    newEntry.entry1[2] = stoi(seg1.at(2));

    newEntry.entry2[0] = stoi(seg2.at(0));
    newEntry.entry2[1] = stoi(seg2.at(1));
    newEntry.entry2[2] = stoi(seg2.at(2));

    return newEntry;
}

//parses to a mesh entry and outputs to the output stream
void generateMesh(ofstream *file, vector<Vector4> verticies, vector<Vector3> texCoordinates, vector<Vector3> norm, vector<fIndex> data)
{
    int duplicates = 0;
    vector<int> point = vector<int>();
    vector<int> pointTex = vector<int>();
    vector<int> pointNorm = vector<int>();

    vector<Vector3> normAdj = vector<Vector3>();
    // step 1. find the number of unique v1/vt1/vn entries
    for (int i = 0; i < data.size(); i++)
    {
        int existPosition = -1;
        if (!alreadyExists(point, data.at(i).entry0, existPosition))
        {
            point.push_back(data.at(i).entry0[0]);
            pointTex.push_back(data.at(i).entry0[1]);
            pointNorm.push_back(data.at(i).entry0[2]);

            Vector3 newNorm;
            newNorm.x = 0;
            newNorm.y = 0;
            newNorm.z = 0;
            normAdj.push_back(newNorm);
        }
        else
        {
            //cout << "already exists at index " << existPosition << endl;
            //if it exists, try adding normals and texture coords together
            normAdj.at(existPosition).x += norm.at(data.at(pointNorm.at(existPosition)).entry0[2]).x;
            normAdj.at(existPosition).y += norm.at(data.at(pointNorm.at(existPosition)).entry0[2]).y;
            normAdj.at(existPosition).z += norm.at(data.at(pointNorm.at(existPosition)).entry0[2]).z;
            //spoiler: it didn't work. this is not the way
            duplicates++;
        }

        if (!alreadyExists(point, data.at(i).entry1, existPosition))
        {
            point.push_back(data.at(i).entry1[0]);
            pointTex.push_back(data.at(i).entry1[1]);
            pointNorm.push_back(data.at(i).entry1[2]);

            Vector3 newNorm;
            newNorm.x = 0;
            newNorm.y = 0;
            newNorm.z = 0;
            normAdj.push_back(newNorm);
        }
        else
        {
            //cout << "already exists at index " << existPosition << endl;
            //if it exists, try adding normals and texture coords together
            normAdj.at(existPosition).x += norm.at(data.at(pointNorm.at(existPosition)).entry1[2]).x;
            normAdj.at(existPosition).y += norm.at(data.at(pointNorm.at(existPosition)).entry1[2]).y;
            normAdj.at(existPosition).z += norm.at(data.at(pointNorm.at(existPosition)).entry1[2]).z;
            //spoiler: it didn't work. this is not the way
            duplicates++;
        }

        if (!alreadyExists(point, data.at(i).entry2, existPosition))
        {
            point.push_back(data.at(i).entry2[0]);
            pointTex.push_back(data.at(i).entry2[1]);
            pointNorm.push_back(data.at(i).entry2[2]);

            Vector3 newNorm;
            newNorm.x = 0;
            newNorm.y = 0;
            newNorm.z = 0;
            normAdj.push_back(newNorm);
        }
        else
        {
            //cout << "already exists at index " << existPosition << endl;
            //if it exists, try adding normals and texture coords together
            normAdj.at(existPosition).x += norm.at(data.at(pointNorm.at(existPosition)).entry2[2]).x;
            normAdj.at(existPosition).y += norm.at(data.at(pointNorm.at(existPosition)).entry2[2]).y;
            normAdj.at(existPosition).z += norm.at(data.at(pointNorm.at(existPosition)).entry2[2]).z;
            //spoiler: it didn't work. this is not the way

            duplicates++;
        }
    }

    cout << "there are " << duplicates << " duplicate points" << endl;

    //ok, take the total size of vector<fIndex> data and multiply it by 3. This number - duplicates = verticies. If this is not the case, there is an error somewhere
    if ((data.size() * 3) - duplicates != verticies.size())
    {
        int debugErrorNum = (data.size() * 3) - duplicates;
        cout << "Error. (data size() * 3) - duplicates = " << debugErrorNum << ". Expected " << verticies.size() << ". I don't really know what to tell you other than: something somewhere is fucked" << endl;
    }


    //step 2. Now we have all the information we need to generate the vertex elements inside a sins of a solar empire mesh file
    *file << char(9) << "NumVertices " << verticies.size() << endl;
    *file <<setprecision(6)<<fixed;
    for (int i = 0; i < point.size(); i++)
    {
        *file << char(9) <<"Vertex" << endl;
        *file << char(9) << char(9) <<"Position [ " << verticies.at(point.at(i)-1).x << " " << verticies.at(point.at(i)-1).y << " " << verticies.at(point.at(i)-1).z << " ]" << endl;
        *file << char(9) << char(9) <<"Normal [ " << norm.at(pointNorm.at(i)-1).x + normAdj.at(pointNorm.at(i)-1).x << " " << norm.at(pointNorm.at(i)-1).y + normAdj.at(pointNorm.at(i)-1).y << " " << norm.at(pointNorm.at(i)-1).z + normAdj.at(pointNorm.at(i)-1).z << " ]" << endl;
        
        //there should never be a normal value of 0 0 0. catch these cases and print them
        if (norm.at(pointNorm.at(i)-1).x == 0 && norm.at(pointNorm.at(i)-1).y == 0 && norm.at(pointNorm.at(i)-1).z == 0)
        {
            cout << "error. vertex " << i << " has a normal value of 0 0 0. pointNorm.at(i) = " << pointNorm.at(i) << endl;
        }

        //i think this part isn't correct. It's texture coordinates when I should be calculating normals
        //*file << char(9) << char(9) <<"Tangent [ " << texCoordinates.at(pointTex.at(i)-1).x << " " << texCoordinates.at(pointTex.at(i)-1).y << " " << texCoordinates.at(pointTex.at(i)-1).z << " ]" << endl;
        *file << char(9) << char(9) <<"Tangent [ 0.000000 0.000000 0.000000 ]" << endl;//this tangent bullshit is really fucking lame

        *file << char(9) << char(9) << "Color 0" << endl ;//always 0

        *file << char(9) << char(9) << "U0 " << texCoordinates.at(pointTex.at(i)-1).x << endl;
        *file << char(9) << char(9) << "V0 " << texCoordinates.at(pointTex.at(i)-1).y << endl;
        *file << char(9) << char(9) << "U1 0.000000" << endl; //this *should* be zero for tris-only meshes //texCoordinates.at(pointTex.at(i)-1).z
        *file << char(9) << char(9) << "V1 0.000000" << endl;
    }

    //step 3. process all the triangle faces and poop them out into the mesh file
    *file << char(9) << "NumTriangles " << data.size() << endl;
    for (int i = 0; i < data.size(); i++)
    {
        *file << char(9) <<"Triangle" << endl;
        *file << char(9) << char(9) << "iVertex0 " << data.at(i).entry0[0]-1 << endl;
        *file << char(9) << char(9) << "iVertex1 " << data.at(i).entry1[0]-1 << endl;
        *file << char(9) << char(9) << "iVertex2 " << data.at(i).entry2[0]-1 << endl;
        *file << char(9) << char(9) << "iMaterial 0" << endl;   //iMaterial is always 0, thank fuck
    }

    /*step 4. Deal with all the trailing bullshit they added at the end
    EX:
    NumCachedVertexIndicesInDirection:UP 2750
	Index 12960
	Index 1123
	Index 1124
	Index 3531
	Index 3542
	Index 3544
    .. and so on and so fourth
    */

    // normal x = -1 == "LEFT"
    // normal x = 1 == "RIGHT"
    // normal y = -1 == "DOWN"
    // normal y = 1 == "UP"
    // normal z = -1 == "BACK"
    // normal z = 1 == "FRONT"

    //i *GUESS* whichever field has the highest normal value is the direction a vertex is "facing"
    vector<int> upVerts = vector<int>();
    vector<int> downVerts = vector<int>();
    vector<int> rightVerts = vector<int>();
    vector<int> leftVerts = vector<int>();
    vector<int> frontVerts = vector<int>();
    vector<int> backVerts = vector<int>();

    //for each vertex, figure out which direction it's facing the most and add that direction into the corresponding vector
    for (int i = 0; i < point.size(); i++)
    {
        //figure out which is larger; the normal abs(x), the normal abs(y) or the normal abs(z) and then save the index of that vertex
        int largestNum = largestInt(abs(norm.at(pointNorm.at(i)).x), abs(norm.at(pointNorm.at(i)).y), abs(norm.at(pointNorm.at(i)).z));
        if (largestNum == abs(norm.at(pointNorm.at(i)).x))
        {
            if (norm.at(pointNorm.at(i)).x > 0)
            {
                rightVerts.push_back(i);
            }
            else
            {
                leftVerts.push_back(i);
            }
        }
        else if (largestNum == abs(norm.at(pointNorm.at(i)).y))
        {
            if (norm.at(pointNorm.at(i)).y > 0)
            {
                upVerts.push_back(i);
            }
            else
            {
                downVerts.push_back(i);
            }
        }
        else
        {
            if (norm.at(pointNorm.at(i)).z > 0)
            {
                frontVerts.push_back(i);
            }
            else
            {
                backVerts.push_back(i);
            }
        }
    }
    
    //there. Now that we have THAT straightened out, time to add the vertex directional information into the file
    //the order is: up, down, left, right, front, back

    *file << char(9) << "NumCachedVertexIndicesInDirection:UP " << upVerts.size() << endl;
    for (int i = 0; i < upVerts.size(); i++)
    {
        *file << char(9) << "Index " << upVerts.at(i) << endl;
    }

    *file << char(9) << "NumCachedVertexIndicesInDirection:DOWN " << downVerts.size() << endl;
    for (int i = 0; i < downVerts.size(); i++)
    {
        *file << char(9) << "Index " << downVerts.at(i) << endl;
    }

    *file << char(9) << "NumCachedVertexIndicesInDirection:LEFT " << leftVerts.size() << endl;
    for (int i = 0; i < leftVerts.size(); i++)
    {
        *file << char(9) << "Index " << leftVerts.at(i) << endl;
    }

    *file << char(9) << "NumCachedVertexIndicesInDirection:RIGHT " << rightVerts.size() << endl;
    for (int i = 0; i < rightVerts.size(); i++)
    {
        *file << char(9) << "Index " << rightVerts.at(i) << endl;
    }

    *file << char(9) << "NumCachedVertexIndicesInDirection:FRONT " << frontVerts.size() << endl;
    for (int i = 0; i < frontVerts.size(); i++)
    {
        *file << char(9) << "Index " << frontVerts.at(i) << endl;
    }

    *file << char(9) << "NumCachedVertexIndicesInDirection:BACK " << backVerts.size() << endl;
    for (int i = 0; i < backVerts.size(); i++)
    {
        *file << char(9) << "Index " << backVerts.at(i) << endl;
    }
}

//instead of making triangles out of points, make points of of triangles. It's not stupid if it works
void generateMeshBetter(ofstream *file, vector<Vector4> verticies, vector<Vector3> texCoordinates, vector<Vector3> norm, vector<fIndex> data, vector<string> specialPointNames, vector<Vector3> specialPointPositions, vector<Vector3> specialRotations)
{
    //steps:
    //step 1. find the number of unique v/vt/vn entries. or just pretent they're all unique, should still work, there will just be 50% more verticies
    //step 1.5. add the special point data.
    //step 2. Now we have all the information we need to generate the vertex elements inside a sins of a solar empire mesh file
    //step 3. process all the triangle faces and poop them out into the mesh file
    //step 4. Deal with all the trailing bullshit they added at the end

    //step 1. find the number of unique v/vt/vn entries. or just pretent they're all unique, should still work, there will just be 50% more verticies
    vector<meshVertex> triangleCoords = vector<meshVertex>();
    vector<triangleEntry> tris = vector<triangleEntry>();
    for (int i = 0; i < data.size(); i++)
    {
        // each fIndex data has 3 entries
        meshVertex v1;
        meshVertex v2;
        meshVertex v3;

        v1.position.x = verticies.at(data.at(i).entry0[0]-1).x;
        v1.position.y = verticies.at(data.at(i).entry0[0]-1).y;
        v1.position.z = verticies.at(data.at(i).entry0[0]-1).z;
        v1.U0 = texCoordinates.at(data.at(i).entry0[1]-1).x;
        v1.V0 = texCoordinates.at(data.at(i).entry0[1]-1).y;
        v1.U1 = texCoordinates.at(data.at(i).entry0[1]-1).x;
        v1.V1 = texCoordinates.at(data.at(i).entry0[1]-1).y;
        v1.Normal.x = norm.at(data.at(i).entry0[2]-1).x;
        v1.Normal.y = norm.at(data.at(i).entry0[2]-1).y;
        v1.Normal.z = norm.at(data.at(i).entry0[2]-1).z;
        triangleCoords.push_back(v1);


        v2.position.x = verticies.at(data.at(i).entry1[0]-1).x;
        v2.position.y = verticies.at(data.at(i).entry1[0]-1).y;
        v2.position.z = verticies.at(data.at(i).entry1[0]-1).z;
        v2.U0 = texCoordinates.at(data.at(i).entry1[1]-1).x;
        v2.V0 = texCoordinates.at(data.at(i).entry1[1]-1).y;
        v2.U1 = texCoordinates.at(data.at(i).entry1[1]-1).x;
        v2.V1 = texCoordinates.at(data.at(i).entry1[1]-1).y;
        v2.Normal.x = norm.at(data.at(i).entry1[2]-1).x;
        v2.Normal.y = norm.at(data.at(i).entry1[2]-1).y;
        v2.Normal.z = norm.at(data.at(i).entry1[2]-1).z;
        triangleCoords.push_back(v2);

        v3.position.x = verticies.at(data.at(i).entry2[0]-1).x;
        v3.position.y = verticies.at(data.at(i).entry2[0]-1).y;
        v3.position.z = verticies.at(data.at(i).entry2[0]-1).z;
        v3.U0 = texCoordinates.at(data.at(i).entry2[1]-1).x;
        v3.V0 = texCoordinates.at(data.at(i).entry2[1]-1).y;
        v3.U1 = texCoordinates.at(data.at(i).entry2[1]-1).x;
        v3.V1 = texCoordinates.at(data.at(i).entry2[1]-1).y;
        v3.Normal.x = norm.at(data.at(i).entry2[2]-1).x;
        v3.Normal.y = norm.at(data.at(i).entry2[2]-1).y;
        v3.Normal.z = norm.at(data.at(i).entry2[2]-1).z;
        triangleCoords.push_back(v3);


        triangleEntry currentTri;
        currentTri.indexMat = 0;
        currentTri.indexVert0 = (i * 3);
        currentTri.indexVert1 = (i * 3) + 1;
        currentTri.indexVert2 = (i * 3) + 2;
        tris.push_back(currentTri);

    }

    cout << "generated " << tris.size() << " tris and " << triangleCoords.size() << " .mesh compatible vertices" << endl;

    //step 1.5. add the special point data.
    //yes, I know this code is messy. Too bad.
    *file << setprecision(6)<<fixed;
    *file << char(9) << "NumPoints " << specialPointNames.size() << endl;
    for (int i = 0; i < specialPointNames.size(); i++)
    {
        pointToFilestream(file, specialPointNames.at(i), specialPointPositions.at(i), specialRotations.at(i));
    }

    // step 2. Now we have all the information we need to generate the vertex elements inside a sins of a solar empire mesh file
    *file << char(9) << "NumVertices " << triangleCoords.size() << endl;
    int numNormalErrors = 0;
    for (int i = 0; i < triangleCoords.size(); i++)
    {
        *file << char(9) <<"Vertex" << endl;
        *file << char(9) << char(9) <<"Position [ " << triangleCoords.at(i).position.x << " " << triangleCoords.at(i).position.y << " " << triangleCoords.at(i).position.z << " ]" << endl;
        *file << char(9) << char(9) <<"Normal [ " << triangleCoords.at(i).Normal.x << " " << triangleCoords.at(i).Normal.y << " " << triangleCoords.at(i).Normal.z << " ]" << endl;
        
        //there should never be a normal value of 0 0 0. catch these cases and print them
        if (triangleCoords.at(i).Normal.x == 0 && triangleCoords.at(i).Normal.y == 0 && triangleCoords.at(i).Normal.z == 0 && LOG_LEVEL > 0)
        {
            cout << "vertex " << i << " has a normal value of 0 0 0." << endl;
            numNormalErrors++;
        }

        //i think this part isn't correct. It's texture coordinates when I should be calculating normals
        //*file << char(9) << char(9) <<"Tangent [ " << texCoordinates.at(pointTex.at(i)-1).x << " " << texCoordinates.at(pointTex.at(i)-1).y << " " << texCoordinates.at(pointTex.at(i)-1).z << " ]" << endl;
        *file << char(9) << char(9) <<"Tangent [ 0.000000 0.000000 0.000000 ]" << endl;//this tangent bullshit is really fucking lame

        *file << char(9) << char(9) << "Color 0" << endl ;//always 0

        *file << char(9) << char(9) << "U0 " << triangleCoords.at(i).U0 << endl;
        *file << char(9) << char(9) << "V0 " << 1-(triangleCoords.at(i).V0) << endl;//do this or the uvs will be flipped
        *file << char(9) << char(9) << "U1 " << triangleCoords.at(i).U1 << endl;
        *file << char(9) << char(9) << "V1 " << triangleCoords.at(i).V1 << endl;
    }

    if (numNormalErrors > 0 && LOG_LEVEL > 0)
    {
        cout << "Don't worry about those vertex normal errors. I don't know how to fix it but luckily it doesn't seem to break anything" << endl;
    }

    //step 3. process all the triangle faces and poop them out into the mesh file
    *file << char(9) << "NumTriangles " << data.size() << endl;
    for (int i = 0; i < data.size(); i++)
    {
        *file << char(9) <<"Triangle" << endl;
        *file << char(9) << char(9) << "iVertex0 " << tris.at(i).indexVert0 << endl;
        *file << char(9) << char(9) << "iVertex1 " << tris.at(i).indexVert1 << endl;
        *file << char(9) << char(9) << "iVertex2 " << tris.at(i).indexVert2 << endl;
        *file << char(9) << char(9) << "iMaterial 0" << endl;   //iMaterial is always 0, thank fuck
    }
    
    //i *GUESS* whichever field has the highest normal value is the direction a vertex is "facing"
    vector<int> upVerts = vector<int>();
    vector<int> downVerts = vector<int>();
    vector<int> rightVerts = vector<int>();
    vector<int> leftVerts = vector<int>();
    vector<int> frontVerts = vector<int>();
    vector<int> backVerts = vector<int>();

    //for each vertex, figure out which direction it's facing the most and add that direction into the corresponding vector
    for (int i = 0; i < triangleCoords.size(); i++)
    {
        //figure out which is larger; the normal abs(x), the normal abs(y) or the normal abs(z) and then save the index of that vertex
        int largestNum = largestInt(abs(triangleCoords.at(i).Normal.x), abs(triangleCoords.at(i).Normal.y), abs(triangleCoords.at(i).Normal.z));
        if (largestNum == abs(triangleCoords.at(i).Normal.x))
        {
            if (triangleCoords.at(i).Normal.x > 0)
            {
                rightVerts.push_back(i);
            }
            else
            {
                leftVerts.push_back(i);
            }
        }
        else if (largestNum == abs(triangleCoords.at(i).Normal.y))
        {
            if (triangleCoords.at(i).Normal.y > 0)
            {
                upVerts.push_back(i);
            }
            else
            {
                downVerts.push_back(i);
            }
        }
        else
        {
            if (triangleCoords.at(i).Normal.z > 0)
            {
                frontVerts.push_back(i);
            }
            else
            {
                backVerts.push_back(i);
            }
        }
    }
    
    //there. Now that we have THAT straightened out, time to add the vertex directional information into the file
    //the order is: up, down, left, right, front, back

    *file << char(9) << "NumCachedVertexIndicesInDirection:UP " << upVerts.size() << endl;
    for (int i = 0; i < upVerts.size(); i++)
    {
        *file << char(9) << "Index " << upVerts.at(i) << endl;
    }

    *file << char(9) << "NumCachedVertexIndicesInDirection:DOWN " << downVerts.size() << endl;
    for (int i = 0; i < downVerts.size(); i++)
    {
        *file << char(9) << "Index " << downVerts.at(i) << endl;
    }

    *file << char(9) << "NumCachedVertexIndicesInDirection:LEFT " << leftVerts.size() << endl;
    for (int i = 0; i < leftVerts.size(); i++)
    {
        *file << char(9) << "Index " << leftVerts.at(i) << endl;
    }

    *file << char(9) << "NumCachedVertexIndicesInDirection:RIGHT " << rightVerts.size() << endl;
    for (int i = 0; i < rightVerts.size(); i++)
    {
        *file << char(9) << "Index " << rightVerts.at(i) << endl;
    }

    *file << char(9) << "NumCachedVertexIndicesInDirection:FRONT " << frontVerts.size() << endl;
    for (int i = 0; i < frontVerts.size(); i++)
    {
        *file << char(9) << "Index " << frontVerts.at(i) << endl;
    }

    *file << char(9) << "NumCachedVertexIndicesInDirection:BACK " << backVerts.size() << endl;
    for (int i = 0; i < backVerts.size(); i++)
    {
        *file << char(9) << "Index " << backVerts.at(i) << endl;
    }
}

void pointToFilestream(ofstream *file, string pointName, Vector3 position, Vector3 rotation)
{
    *file << char(9) << "Point" << endl;
    *file << char(9) << char(9) << "DataString " << char(34) << pointName << char(34) << endl;
    *file << char(9) << char(9) << "Position [ " << position.x << " " << position.y << " " << position.z << " ]" << endl;
    *file << char(9) << char(9) << "Orientation" << endl;

    *file << char(9) << char(9) << char(9) << " [ " << rotation.x << " 0.000000 0.000000 ]" << endl;
    *file << char(9) << char(9) << char(9) << " [ 0.000000 " << rotation.y << " 0.000000 ]" << endl;
    *file << char(9) << char(9) << char(9) << " [ 0.000000 0.000000 " << rotation.z << " ]" << endl;
}

bool alreadyExists(vector<int> points, int *pointToCheck, int &existPosition)
{
    existPosition = -1;
    bool foundMatch = false;
    int index = 0;
    while (!foundMatch && index < points.size())
    {
        if (pointToCheck[0] == points.at(index))
        {
            existPosition = index;
            foundMatch = true;
        }
        index++;
    }

    return foundMatch;
}

//returns the largest of 3 ints
int largestInt(int i1, int i2, int i3)
{
    int ret = max(i1,i2);
    ret = max(ret, i3);
    return ret;
}

bool isSpecialPoint(string input)
{
    if (input.find("Ability-") != std::string::npos)
    {
        return true;
    }
    else if (input.find("Above") != std::string::npos)
    {
        return true;
    }
    else if (input.find("Aura") != std::string::npos)
    {
        return true;
    }
    else if (input.find("Center") != std::string::npos)
    {
        return true;
    }
    else if (input.find("Weapon-") != std::string::npos)
    {
        return true;
    }
    else if (input.find("Exhaust") != std::string::npos)
    {
        return true;
    }

    return false;
}

//in blender, objects are always in the format of [name]_[name of vertex array]. This function chops of anything after an underscore
string trimStringToSOSEPoint(string input)
{
    string newString = input;
    int underscorePos = input.find("_");
    if (underscorePos != std::string::npos)
    {
        newString = input.substr(0, underscorePos);
    }

    return newString;
}

/*
SOSE mesh files contain:
[game block]
NumVerticies int
[vertex]
    position [ x,y,z ]
    Normal [ float, float, float ]
    Tangent [ float, float, float ]
    Color 0
    U0 float
    V0 float
    U1 float
    V1 float
...
NumTriangles int
    Triangle
        iVertex0 int
        iVertex1 int
        iVertex2 int
        iMaterial 0
*/