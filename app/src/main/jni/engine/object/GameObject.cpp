//
// Created by piotr on 31/08/16.
//

#include <GLES2/gl2.h>
#include "GameObject.h"
#include "../engine.h"

void GameObject::init(float centerX1, float centerY1, float centerZ1, float boxSize) {

    mCenterX =centerX1;
    mCenterY =centerY1;
    mCenterZ =centerZ1;
    mObjectSize =boxSize;
}

void GameObject::draw() {




        glEnable(GL_CULL_FACE);
        Engine_glBegin(GL_TRIANGLE_STRIP);
        Engine_glColor3f(1, 0, 0);
        //bottom
        Engine_glVertex3f(mCenterX - mObjectSize, mCenterZ, mCenterY - mObjectSize); // tl
        Engine_glVertex3f(mCenterX - mObjectSize, mCenterZ, mCenterY + mObjectSize); // bl
        Engine_glVertex3f(mCenterX + mObjectSize, mCenterZ, mCenterY - mObjectSize); // tr
        Engine_glVertex3f(mCenterX + mObjectSize, mCenterZ, mCenterY + mObjectSize); // br
        //top
        //Engine_glColor3f(0, 1, 0);

        Engine_glVertex3f(mCenterX - mObjectSize, mCenterZ + mObjectSize * 2,
                          mCenterY - mObjectSize); // tl
        Engine_glVertex3f(mCenterX - mObjectSize, mCenterZ + mObjectSize * 2,
                          mCenterY + mObjectSize); // bl
        Engine_glVertex3f(mCenterX + mObjectSize, mCenterZ + mObjectSize * 2,
                          mCenterY - mObjectSize); // tr
        Engine_glVertex3f(mCenterX + mObjectSize, mCenterZ + mObjectSize * 2,
                          mCenterY + mObjectSize); // br
        //front
        //Engine_glColor3f(0, 0, 1);
        Engine_glVertex3f(mCenterX - mObjectSize, mCenterZ, mCenterY + mObjectSize); // bl
        Engine_glVertex3f(mCenterX + mObjectSize, mCenterZ, mCenterY + mObjectSize); // br
        Engine_glVertex3f(mCenterX - mObjectSize, mCenterZ + mObjectSize * 2,
                          mCenterY + mObjectSize); // tl
        Engine_glVertex3f(mCenterX + mObjectSize, mCenterZ + mObjectSize * 2,
                          mCenterY + mObjectSize); // tr
        //back
        Engine_glVertex3f(mCenterX - mObjectSize, mCenterZ, mCenterY - mObjectSize); // tl
        Engine_glVertex3f(mCenterX + mObjectSize, mCenterZ, mCenterY - mObjectSize); // tr
        Engine_glVertex3f(mCenterX - mObjectSize, mCenterZ + mObjectSize * 2,
                          mCenterY - mObjectSize); // tl
        Engine_glVertex3f(mCenterX + mObjectSize, mCenterZ + mObjectSize * 2,
                          mCenterY - mObjectSize); // tr
        //left
        Engine_glVertex3f(mCenterX - mObjectSize, mCenterZ, mCenterY - mObjectSize); // tl
        Engine_glVertex3f(mCenterX - mObjectSize, mCenterZ, mCenterY + mObjectSize); // bl
        Engine_glVertex3f(mCenterX - mObjectSize, mCenterZ + mObjectSize * 2,
                          mCenterY - mObjectSize); // tl
        Engine_glVertex3f(mCenterX - mObjectSize, mCenterZ + mObjectSize * 2,
                          mCenterY + mObjectSize); // bl
        //right
        Engine_glVertex3f(mCenterX + mObjectSize, mCenterZ, mCenterY - mObjectSize); // tr
        Engine_glVertex3f(mCenterX + mObjectSize, mCenterZ, mCenterY + mObjectSize); // br
        Engine_glVertex3f(mCenterX + mObjectSize, mCenterZ + mObjectSize * 2,
                          mCenterY - mObjectSize); // tr
        Engine_glVertex3f(mCenterX + mObjectSize, mCenterZ + mObjectSize * 2,
                          mCenterY + mObjectSize); // br

        Engine_glEnd();
        glDisable(GL_CULL_FACE);








}

void GameObject::setPosition(float x, float y) {
        mCenterX =x;
        mCenterY =y;
}

GameObject::GameObject() {

}
