#ifndef __PROGTEST__
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cctype>
#include <climits>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>
#include <algorithm>
#include <set>
#include <queue>
#include <memory>
#include <functional>
#include <stdexcept>
using namespace std;
#define MYDEBUG
//#define PRINTTABLE
//#define PRINTTREE
#endif /* __PROGTEST__ */

class Tree //binary tree for storing values for huffman de/encoding
{
  public:
  struct Node
  {
    int symbol; 
    Node *left, 
         *right;
    Node() : symbol(0), left(nullptr), right(nullptr) {}
    Node(int insert) : symbol(insert), left(nullptr), right(nullptr) {}
    ~Node()
    {
      delete left;
      delete right;
    }
  };
  void print2D(const Tree::Node *root) const; //debug function to print the tree
  private:
  void print2DRec(const Node *root, int space) const;
};

class Huffman
{
  public:
    int makeTable(ifstream* Input); //save the preorder traversal to an array
    struct Tree::Node* makeTree(const vector<int> preorder, int n); //make a decoding tree from the array
    void decode(ifstream* Input, ofstream* Output); //decode according to the tree
    void initialize(ifstream* input); //prepare everything to decode
    void printTable() const; //debug function to print table
    //Huffman() : Root(new Tree::Node()) {}
    ~Huffman()
    {
      if(initialized) delete Root;
    }
  private:
    int readBit(ifstream* Input);
    int readChar(ifstream* Input);
    struct Tree::Node* makeTreeRec(const vector<int> preorder, int *index_ptr, int n);
    vector<int> mTable;
    int mBytePosition;
    int mByte = 0;
    bool initialized = false;
    Tree::Node *Root;
};

//debug function to print Tree
void Tree::print2DRec(const Tree::Node *root, int space) const
{  
    if (root == NULL)  
        return;  
    space += 3;  
    Tree::print2DRec(root->right, space);  
    cout<<endl;  
    for (int i = 3; i < space; i++)  
        cout<<" ";  
    cout<<(char)root->symbol<<"\n";  
    Tree::print2DRec(root->left, space);  
}  
  
// Wrapper over print2DRec()  
void Tree::print2D(const Tree::Node *root) const
{  
    Tree::print2DRec(root, 0);  
}

//debug function to print table
void Huffman::printTable() const
{
  std::cout << "N ";
  for (int i = 1; i < (int)mTable.size(); i++)
    std::cout << i << ' ';
  std::cout << endl;
  for (std::vector<int>::const_iterator i = mTable.begin(); i != mTable.end(); ++i)
    std::cout << (char)*i << ' ';
  std::cout << endl;
}


/* A recursive function to create a Binary Tree from given array. The function returns root of tree. index_ptr is used 
   to update index values in recursive calls. index must be initially 
   passed as 0 */
struct Tree::Node *Huffman::makeTreeRec(const vector<int> preorder, int *index_ptr, int n) 
{ 
    int index = *index_ptr; // store the current value of index in preorder 
  
    // Base Case: All nodes are constructed 
    if (index == n) 
        return NULL; 
  
    // Allocate memory for this node and increment index for 
    // subsequent recursive calls 
    struct Tree::Node *temp = new Tree::Node(preorder[index]); 
    (*index_ptr)++; 
  
    // If this is an internal node, construct left and right subtrees and link the subtrees 
    if (preorder[index] == -1) 
    { 
      temp->left  = makeTreeRec(preorder, index_ptr, n); 
      temp->right = makeTreeRec(preorder, index_ptr, n); 
    } 
  
    return temp; 
} 
  
// A wrapper over constructTreeUtil() 
struct Tree::Node *Huffman::makeTree(const vector<int> preorder, int n) 
{ 
    // Initialize index as 0. Value of index is used in recursion to maintain 
    // the current index in preorder. 
    int index = 0; 
  
    return makeTreeRec (preorder, &index, n); 
}

//read one bit from the current byte, update the position accordingly
int Huffman::readBit(ifstream* Input)
{
  int bit = 0;
  
  if (mBytePosition == 8)
  {
    mBytePosition = 0;
    Input->read((char*)&mByte,1);
    if(Input->eof())
    {
      Input->setstate(ifstream::badbit);
    }    
  }

  bit = mByte;
  bit = bit >> (7 - mBytePosition);
  bit = bit & 1;
  mBytePosition++;
  
  return bit;
}

//read the next character from input
int Huffman::readChar(ifstream* Input)
{
  unsigned int character = mByte;

  Input->read((char*)&mByte,1);
  if(Input->eof())
  {
    Input->setstate(ifstream::badbit);
  }
  if(mBytePosition == 8) return (int)mByte;


  character = (character << mBytePosition) & 0xFF;
  if (mBytePosition)
  {
    character = character | ((int)mByte >> (8 - mBytePosition));
  }

  return character;
}

//read the preorder traversal of a full binary tree to an array
int Huffman::makeTable(ifstream* Input)
{
   int TerminateCounter = 1,
       Bit = 0;

   if(!Input->read((char*)&mByte,1)) return false;
   mBytePosition = 0;

   mTable.push_back(-1);
   Huffman::readBit(Input);
   TerminateCounter++;

   while(1)
   {
     Bit = Huffman::readBit(Input);
     if(Bit == 1)
     {
       mTable.push_back(Huffman::readChar(Input));
       TerminateCounter--;
     }
     else //Bit == 0
     {
       mTable.push_back(-1);
       TerminateCounter++;
     }
     if(TerminateCounter == 0) return true;
   }
}

void Huffman::initialize(ifstream* Input)
{
  makeTable(Input);
  
  #ifdef PRINTTABLE
    printTable();
  #endif //PRINTTABLE
  
  Root = makeTree(mTable,(int)mTable.size());
  
  #ifdef PRINTTREE
    Tree Debug;
    Debug.print2D(Root);
  #endif //PRINTTREE

  initialized = true;
}

//decode the input by a stored binary tree
void Huffman::decode(ifstream* Input, ofstream* Output)
{
  int SectorSize = 4096;
  int HuffCounter = 0; //counts how many symbols were decoded in this sector
  int Bit = 0; //value of current bit being read
  Tree::Node* CurrentNode;


  
  while(1)
  {
    if (! Huffman::readBit(Input)) //calculating the smaller sector size
    {
      SectorSize = (Huffman::readChar(Input) << 4);
      for (int i = 0; i < 4; i++)
      {
        SectorSize = SectorSize | ( Huffman::readBit(Input) << ( 3 - i ) );
      }
    }
    else SectorSize = 4096;
    //read the sector size as per specification
    


    while (HuffCounter < SectorSize)
    {
      CurrentNode = Root;
      while(1)
      {
        Bit=Huffman::readBit(Input);
        if(Bit)
        {
          CurrentNode = CurrentNode->right;
        }
        else
        {
          CurrentNode = CurrentNode->left;
        }
        if(CurrentNode->symbol != -1)
        {
          *Output << (char)CurrentNode->symbol;
          HuffCounter++;
          break;
        }
      }
    }
    // Written the sector
    HuffCounter = 0;
    if(SectorSize != 4096) break;
  }
}

bool decompressFile ( const char * inFileName, const char * outFileName )
{
  ifstream Input;
  ofstream Output;
  Huffman Huff;
  
  Input.exceptions ( ifstream::badbit | ifstream::failbit | ifstream::eofbit );
  Output.exceptions ( ifstream::badbit | ifstream::failbit | ifstream::eofbit );

  try
  {
    Input.open(inFileName,std::ifstream::binary);
  }
  catch(const ifstream::failure& e)
  {
    return false;
  }
  try
  {
    Output.open(outFileName,std::ofstream::binary);
  }
  catch(const ofstream::failure& e)
  {
    return false;
  }

  try
  {
    Huff.initialize(&Input);
    Huff.decode(&Input, &Output);
  }
  catch (const ios_base::failure& e) {
    return false;
  }


  Input.close();
  Output.close();
  return true;
}


//----------------------------------------------------------------------------------------------------------
bool compressFile ( const char * inFileName, const char * outFileName )
{
  // keep this dummy implementation (no bonus) or implement the compression (bonus)
  return false;
}
#ifndef __PROGTEST__
bool identicalFiles ( const char * fileName1, const char * fileName2 )
{
  std::ifstream file1(fileName1, std::ifstream::ate | std::ifstream::binary); //open file at the end
  std::ifstream file2(fileName2, std::ifstream::ate | std::ifstream::binary); //open file at the end
  const std::ifstream::pos_type fileSize = file1.tellg();

  if (fileSize != file2.tellg()) {
      return false; //different file size
  }

  file1.seekg(0); //rewind
  file2.seekg(0); //rewind

  std::istreambuf_iterator<char> begin1(file1);
  std::istreambuf_iterator<char> begin2(file2);

  return std::equal(begin1,std::istreambuf_iterator<char>(),begin2); //Second argument is end-of-range iterator
}








int main ( void )

{

  assert ( decompressFile ( "tests/test0.huf", "tempfile" ) );
  assert ( identicalFiles ( "tests/test0.orig", "tempfile" ) );
  
  assert ( decompressFile ( "tests/test1.huf", "tempfile" ) );
  assert ( identicalFiles ( "tests/test1.orig", "tempfile" ) );

  assert ( decompressFile ( "tests/test2.huf", "tempfile" ) );
  assert ( identicalFiles ( "tests/test2.orig", "tempfile" ) );

  assert ( decompressFile ( "tests/test3.huf", "tempfile" ) );
  assert ( identicalFiles ( "tests/test3.orig", "tempfile" ) );

  assert ( decompressFile ( "tests/test4.huf", "tempfile" ) );
  assert ( identicalFiles ( "tests/test4.orig", "tempfile" ) );

#ifdef MYDEBUG

  assert ( ! decompressFile ( "tests/test99.huf", "tempfile" ) );
  assert ( ! decompressFile ( "tests/test6.huf", "tempfile" ) );
  assert ( ! decompressFile ( "tests/test7.huf", "tempfile" ) );

#endif


  assert ( ! decompressFile ( "tests/test5.huf", "tempfile" ) );

/*
  assert ( decompressFile ( "tests/extra0.huf", "tempfile" ) );
  assert ( identicalFiles ( "tests/extra0.orig", "tempfile" ) );

  assert ( decompressFile ( "tests/extra1.huf", "tempfile" ) );
  assert ( identicalFiles ( "tests/extra1.orig", "tempfile" ) );

  assert ( decompressFile ( "tests/extra2.huf", "tempfile" ) );
  assert ( identicalFiles ( "tests/extra2.orig", "tempfile" ) );

  assert ( decompressFile ( "tests/extra3.huf", "tempfile" ) );
  assert ( identicalFiles ( "tests/extra3.orig", "tempfile" ) );

  assert ( decompressFile ( "tests/extra4.huf", "tempfile" ) );
  assert ( identicalFiles ( "tests/extra4.orig", "tempfile" ) );

  assert ( decompressFile ( "tests/extra5.huf", "tempfile" ) );
  assert ( identicalFiles ( "tests/extra5.orig", "tempfile" ) );

  assert ( decompressFile ( "tests/extra6.huf", "tempfile" ) );
  assert ( identicalFiles ( "tests/extra6.orig", "tempfile" ) );

  assert ( decompressFile ( "tests/extra7.huf", "tempfile" ) );
  assert ( identicalFiles ( "tests/extra7.orig", "tempfile" ) );

  assert ( decompressFile ( "tests/extra8.huf", "tempfile" ) );
  assert ( identicalFiles ( "tests/extra8.orig", "tempfile" ) );

  assert ( decompressFile ( "tests/extra9.huf", "tempfile" ) );
  assert ( identicalFiles ( "tests/extra9.orig", "tempfile" ) );

  */
  return 0;
}
#endif /* __PROGTEST__ */
