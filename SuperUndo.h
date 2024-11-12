#pragma once
/**
* author: Owen Fabula
**/
#include <iostream>
#include <vector>
using namespace std;

/* \brief parent class for UndoPaint and XPaint; exists so that both
   subclasses can be stored in the same mask_state vector.*/
class SuperUndo {
private:
    long xCoord, yCoord, time; //x,y of init point; timestamp for when object was drawn/erased
    int len; //length of XPaint objects
    int act; //action associated with object
public:
    //constructors
    //default constructor
    SuperUndo() { xCoord = yCoord = 0; len = 0; act = -1; time = 0; }
    //fully qualified constructor
    /* \param x anchor x coordinate
       \param y anchor y coordinate
       \param a action associated with object
       \param t timestamp of object*/
    SuperUndo(long x, long y, int a, long t) {
        xCoord = x;
        yCoord = y;
        act = a;
        time = t;
        len = 0;
    }
    //setters/getters
    //accessors
    /* \brief accessor for x coord*/
    long getX() { return xCoord; }
    /* \brief accessor for y coord*/
    long getY() { return yCoord; }
    /* \brief accessor for object length*/
    int getLen() { return len; }
    /* \brief accessor for object's associated action*/
    int getAct() { return act; }
    /* \brief accessor for timestamp*/
    long getTime() { return time; }
    //mutators
    /* \brief mutator for x coord
       \param x new x coord*/
    void setX(long x) { xCoord = x; }
    /* \brief mutator for y coord
       \param y new y coord*/
    void setY(long y) { yCoord = y; }
    /* \brief mutator for pbject length
       \param l new length*/
    void setLen(int l) { len = l;  }
    /* \brief mutator for action associated with object
       \param a new object action*/
    void setAct(int a) { act = a; }
    /* \brief mutator for object timestamp
       \param t new timestamp*/
    void setTime(long t) { time = t; }
    //other methods
    /* \brief pretty print for SuperUndo objects (is not used)*/
    virtual void print() {
        cout << "(x=" << xCoord << ",y=" << yCoord << "), l=" << len << endl;
    }
    //the following functions are virtual because either UndoPaint or XPaint objects
    //need to access them while existing as pointers to SuperUndo in mask_state.
    //their functions will be described as they are concretely implemented below.
    virtual void addPoint(long x, long y) = 0;
    virtual long getPointX(int index) = 0;
    virtual long getPointY(int index) = 0;
    virtual int getPointBrush(int index) = 0;
    virtual int getPointLen(int index) = 0;
    virtual void setPointLen(int index, int val) = 0;
    virtual int getBrushSize() = 0;
    //destructor MUST BE VIRTUAL
    virtual ~SuperUndo() { cout << "in SuperUndo dtor" << endl; xCoord = 0; yCoord = 0; }
};

/* \brief objects of UndoPaint are created when the user paints/erases on the mask with a positive nonzero brush size.*/
class UndoPaint : public SuperUndo {
private:
    int brushSize;
public:
    //constructors
    //default constructor
    UndoPaint() : SuperUndo() { brushSize = 0; }
    //fully qualified constructor
    /* \param x anchor x coordinate
       \param y anchor y coordinate
       \param a action associated with object
       \param t timestamp of object*/
    UndoPaint(long x, long y, int b, int a, long t) : SuperUndo(x, y, a, t) {
        brushSize = b;
    }
    //setters/getters
    //accessors
    /*brief accessor for brush size*/
    int getBrush() { return brushSize; }
    //mutators
    /* \brief mutator for brush size
       \param b new brush size*/
    void setBrush(int b) { brushSize = b; }
    //other methods
    /* \brief pretty print for UndoPaint objects (not used)*/
    void print() override {
        cout << "(x=" << this->getX() << ",y=" << this->getY() << ",b=" << brushSize << ")" << endl;
    }
    /* \brief returns brush size of object. used when needing this info from UndoPaint objects in mask_state*/
    int getBrushSize() override {
        return this->brushSize;
    }
    //just do dummy implementations so UndoPaint is not an abstract class
    void addPoint(long x, long y) override { cout << x << y << endl; }
    long getPointX(int index) override { long i = 0; return i; }
    long getPointY(int index) override { long i = 0; return i; }
    int getPointBrush(int index) override { int i = 100000; return i; }
    int getPointLen(int index) override { int i = 10000; return i; }
    void setPointLen(int index, int val) override { return; }

     //destructor
    ~UndoPaint() { cout << "in UndoPaint dtor" << endl; brushSize = 0; }
};

/* \brief objects of XPaint are created when the user paints/erases on the mask with brush size of zero.*/
class XPaint : public SuperUndo {
public:
    vector<UndoPaint*>pointsInX;
    //constructors
    //default constructor
    XPaint() : SuperUndo() {}
    /* \brief constructor for XPaint
       \param x init x coord
       \param y init y coord
       \param t timestamp*/
    XPaint(long x, long y, long t) : SuperUndo(x, y, 0, t) {}
    /* \brief fully qualified constructor for XPaint
       \param x init x coord
       \param y init y coord
       \param a action associate with this object
       \param t timestamp*/
    XPaint(long x, long y, int a, long t) : SuperUndo(x, y, a, t) {}
    //other methods
    /* \brief pretty print for XPaint objects (not used)*/
    void print() override {
        cout << "(x=" << this->getX() << ",y=" << this->getY() << ") ";
        for (UndoPaint* u : pointsInX) {
            cout << "(x=" << u->getX() << ",y=" << u->getY() << ") ";
        }
        cout << endl;
    }
    /* \brief adds a point to the vector pointsInX
       \param x x coord of point
       \param y y coord of point*/
    void addPoint(long x, long y) override {
        UndoPaint* u = new UndoPaint(x, y, 0, -1, 0); //make new UndoPaint obj
        pointsInX.push_back(u); //put it at end of vector
    }
    /* \brief adds a point to the vector pointsInX
       \param u UndoPaint object*/
    void addPoint(UndoPaint* u) {
        pointsInX.push_back(u);
    }
    /* \brief returns x coord of object at index in pointsInX
       \param index ind of desired x coord*/
    long getPointX(int index) override {
        int size = pointsInX.size();
        if (index <= size - 1 && index >= 0) {
            return pointsInX.at(index)->getX();
        }
        else {
            long x = 1;
            return x;
        }
    }
    /* \brief returns y coord of object at index in pointsInX
       \param index ind of desired y coord*/
    long getPointY(int index) override {
        int size = pointsInX.size();
        if (index <= size - 1 && index >= 0) {
            return pointsInX.at(index)->getY();
        }
        else {
            long x = 1;
            return x;
        }
    }
    /* \brief returns brush size of object at index in pointsInX
       \param index ind of desired brush size*/
    int getPointBrush(int index) override {
        return pointsInX.at(index)->getBrushSize();
    }
    /* \brief returns length of object at index in pointsInX
       \param index ind of desired length*/
    int getPointLen(int index) override {
        return pointsInX.at(index)->getLen();
    }
    /* \brief sets length of object at index in pointsInX
       \param index ind of desired length
       \param val new length value*/
    void setPointLen(int index, int val) override {
        pointsInX.at(index)->setLen(val);
    }

    //implement abstract functions
    int getBrushSize() override { int j = 0; return j; }

    //destructor
    ~XPaint() { cout << "in XPaint dtor" << endl; pointsInX.clear(); }
};