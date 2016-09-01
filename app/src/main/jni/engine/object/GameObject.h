//
// Created by piotr on 31/08/16.
//

#ifndef MYVRPET_GAMEOBJECT_H
#define MYVRPET_GAMEOBJECT_H


class GameObject {

public:GameObject();



    float mCenterX;
    float mCenterY;
    float mCenterZ;
    float mObjectSize;

public: void init(float centerX1, float centerY1, float centerZ1, float boxSize);

public :void draw();

public: void setPosition(float x, float y);

};


#endif //MYVRPET_GAMEOBJECT_H
