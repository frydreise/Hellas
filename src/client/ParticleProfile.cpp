// (C) 2016 Tim Gurto

#include <cassert>

#include "Particle.h"
#include "ParticleProfile.h"

ParticleProfile::ParticleProfile(const std::string &id):
_id(id),
_particlesPerSecond(0){}

ParticleProfile::~ParticleProfile(){
    for (const EntityType *variety : _varieties)
        delete variety;
}

void ParticleProfile::addVariety(const std::string &imageFile, const Rect &drawRect){
    _varieties.push_back(new EntityType(drawRect, "Images/Particles/" + imageFile + ".png"));
}

Particle *ParticleProfile::instantiate(const Point &location) const{
    // Choose random variety
    assert (!_varieties.empty());
    size_t varietyIndex = rand() % _varieties.size();
    const EntityType &variety = *_varieties[varietyIndex];

    // Choose random direction, then set location
    double angle = 2 * PI * randDouble();
    double distance = _distance.generate();
    Point locationOffset(cos(angle) * distance, sin(angle) * distance);
    Point startingLoc = location + locationOffset;

    // Choose random direction, then set velocity
    angle = 2 * PI * randDouble();
    double velocity = _velocity.generate();
    Point startingVelocity(cos(angle) * velocity, sin(angle) * velocity);


    return new Particle(startingLoc, variety.image(), variety.drawRect(), startingVelocity,
                        _altitude.generate(), _fallSpeed.generate());
}

size_t ParticleProfile::numParticlesToAdd(double delta) const{
    size_t total = 0;
    double particlesToAdd = _particlesPerSecond * delta;
    if (particlesToAdd >= 1){ // Whole part
        total = static_cast<size_t>(particlesToAdd);
        particlesToAdd -= total;
    }
    if (randDouble() <= particlesToAdd)
        ++total;
    return total;
}