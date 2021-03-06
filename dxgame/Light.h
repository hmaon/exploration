#ifndef LIGHT_H
#define LIGHT_H

#include <DirectXMath.h>

// this struct is mostly for spotlights but can be used to store data for other types of lights as well
struct Light
{
    DirectX::XMFLOAT4 position; // world space
    DirectX::XMFLOAT3 direction;

    float halfAngle; // angle between axis and generatrix (if you pretend the spotlight is a perfect cone)

    // { cos(halfAngle), constant attenuation, linear att., quadratic att. }:
    float cosHalfAngle;
    float constantAttenuation;
    float linearAttenuation;
    float quadraticAttenuation;
    // these used to be an XMFLOAT4 but I wanted to give them names and XMFLOAT4 can't be in a union due to a non-trivial constructor

    DirectX::XMFLOAT4X4 view; // view matrix
    DirectX::XMFLOAT4X4 projection; // Projection matrices for all the lights, >>>including the directional light in the final position x2 LOD<<<

    DirectX::XMFLOAT4 color; // colored lights can hypnotize?

    bool enabled;

    Light(void) : position(0,0,0,0), direction(0,-1,0), halfAngle((float)M_PI*(18.0f/180.0f)), cosHalfAngle(cos(halfAngle)), constantAttenuation(0.95f), linearAttenuation(0),
        quadraticAttenuation(0.025f), color(1,1,1,1), enabled(false)
    {
        updateProjection();
    }

    void setHalfAngle(float ha)
    {
        halfAngle = ha;
        cosHalfAngle = cos(ha);
        updateProjection();
    }

    void updateProjection()
    {
        float angle = min(halfAngle * 2.5f, (DirectX::XM_PIDIV2));
        // halfAngle*2.5 just needs to make sure the projected view encompasses the entire spotlight
        // one might think that halfAngle*2 would be ideal but that looks more like a view inscribed within the circle of the spotlight

        DirectX::XMVECTOR dir = DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&direction));

        if (fabs(dir.m128_f32[1]) > 0.99999f) dir = DirectX::XMVector3Normalize(DirectX::XMVectorAdd(dir, DirectX::XMVectorSet(0.0001f, 0, 0, 0)));  // vector parallel to our "up" direction? fudge it.

        DirectX::XMMATRIX viewMat = DirectX::XMMatrixLookToLH(XMLoadFloat4(&position), dir, DirectX::XMVectorSet(0, 1, 0, 0));
        DirectX::XMMATRIX projMat = DirectX::XMMatrixPerspectiveFovLH(angle, 1.0f /* shadowmap is a square at any resolution */, 0.1f, 1000.0f); // the 1000 far plane should perhaps be configurable?
        DirectX::XMStoreFloat4x4(&projection, projMat);
        DirectX::XMStoreFloat4x4(&view, viewMat);
    }

    void move(DirectX::FXMVECTOR newPosition, DirectX::FXMVECTOR newDirection)
    {
        DirectX::XMStoreFloat4(&position, newPosition);
        DirectX::XMStoreFloat3(&direction, newDirection);
        updateProjection();
        enabled = true;
    }

    inline DirectX::XMMATRIX getVP()
    {
        return DirectX::XMLoadFloat4x4(&view) * DirectX::XMLoadFloat4x4(&projection);
    }
};

#endif