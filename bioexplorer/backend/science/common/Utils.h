/*
    Copyright 2020 - 2024 Blue Brain Project / EPFL

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#pragma once

#include <science/common/Types.h>

#include <platform/core/common/Types.h>

namespace bioexplorer
{
namespace common
{
/**
 * @brief Left trim of a string
 *
 * @param s String to trim
 * @return Trimmed string
 */
std::string& ltrim(std::string& s);

/**
 * @brief Right trim of a string
 *
 * @param s String to trim
 * @return Trimmed string
 */
std::string& rtrim(std::string& s);

/**
 * @brief Left and right trim of a string
 *
 * @param s String to trim
 * @return Trimmed string
 */
std::string& trim(std::string& s);

/**
 * @brief isClipped Determine if a 3d position is inside a volume defined by
 * clipping planes
 *
 * @param position Position to check
 * @param clippingPlanes Clipping planes defining the volume
 * @return True if the position does not belong to the volume, false otherwise
 */
bool isClipped(const core::Vector3d& position, const Vector4ds& clippingPlanes);

/**
 * @brief Set the default transfer function (Unipolar) to a given model
 *
 * @param model Model to which the transfer function should be set
 */
void setDefaultTransferFunction(core::Model& model, const core::Vector2d range = {0.0, 1.0}, const double alpha = 1.0);

/**
 * @brief Get the Clipping Planes from the scene
 *
 * @param scene 3D scene
 * @return Vector4ds List of clipping planes
 */
Vector4ds getClippingPlanes(const core::Scene& scene);

/**
 * @brief Converts a vector of doubles into a 2D vector
 *
 * @param value Vector of doubles
 * @return Vector2d A 2D vector
 */
core::Vector2d doublesToVector2d(const doubles& value);

/**
 * @brief Converts a vector of doubles into a 3D vector
 *
 * @param value Vector of doubles
 * @return Vector3d A 3D vector
 */
core::Vector3d doublesToVector3d(const doubles& value);

/**
 * @brief Converts a 3D vector to a vector of doubles
 *
 * @param value A 3D vector
 * @return Vector3d Vector of doubles
 */
doubles vector3dToDoubles(const core::Vector3d& value);

/**
 * @brief Converts a vector of doubles into a 4D vector
 *
 * @param value Vector of doubles
 * @return Vector3d A 4D vector
 */
core::Vector4d doublesToVector4d(const doubles& value);

/**
 * @brief Converts a vector of doubles into a Quaternion
 *
 * @param values Vector of doubles
 * @return Quaternion A quaternion
 */
core::Quaterniond doublesToQuaterniond(const doubles& values);

/**
 * @brief Converts a vector of doubles into vector of 4D vectors
 *
 * @param values Vector of doubles
 * @return Quaternion A vector of 4D vectors
 */
Vector4ds doublesToVector4ds(const doubles& values);

/**
 * @brief Converts a vector of doubles into molecular system animation details
 *
 * @param value Vector of doubles
 * @return MolecularSystemAnimationDetails The animation details
 */
details::MolecularSystemAnimationDetails doublesToMolecularSystemAnimationDetails(const doubles& values);

/**
 * @brief Converts a vector of doubles into cell animation details
 *
 * @param value Vector of doubles
 * @return CellAnimationDetails The animation details
 */
details::CellAnimationDetails doublesToCellAnimationDetails(const doubles& values);

/**
 * @brief Converts a vector of doubles into neurons report parameters details
 *
 * @param value Vector of doubles
 * @return NeuronsReportParameters The neurons report parameters details
 */
details::NeuronsReportParameters doublesToNeuronsReportParametersDetails(const doubles& values);

/**
 * @brief Returns a position and a rotation of a instance on a sphere using a
 * sphere-filling algorithm
 *
 * @param radius Radius of the sphere
 * @param occurrences Total number of instances
 * @param rnd Randomized occurrence of the instance (optional)
 * @param position Resulting position of the instance on the sphere
 * @param rotation Resulting orientation of the instance on the sphere
 * @param ratio Ratio of coverage of the sphere
 * @return Vector3d
 */
core::Vector3d sphereFilling(const double radius, const uint64_t occurrence, const uint64_t occurrences,
                             const uint64_t rnd, core::Vector3d& position, core::Quaterniond& rotation,
                             const double ratio = 1.0);

/**
 * @brief Splits a string according to the delimiter
 *
 * @param s String to split
 * @param delimiter Delimiter
 * @return std::vector<std::string> Vector of strings
 */
std::vector<std::string> split(const std::string& s, const std::string& delimiter = CONTENTS_DELIMITER);

/**
 * @brief Combine a list of transformations
 *
 * @param transformations List of transformations
 * @return Transformation Result of the combination
 */
core::Transformation combineTransformations(const core::Transformations& transformations);

/**
 * @brief Intersection between a ray and a box
 *
 * @param origin Origin of the ray
 * @param direction Direction of the ray
 * @param box Box
 * @param t0 Initial t of the ray
 * @param t1 Final t of the ray
 * @param t Intersection value of t if an intersection if found
 * @return true The ray intersects with the box
 * @return false The ray does not intersect with the box
 */
bool rayBoxIntersection(const core::Vector3d& origin, const core::Vector3d& direction, const core::Boxd& box,
                        const double t0, const double t1, double& t);

// Volumes
double sphereVolume(const double radius);
double cylinderVolume(const double height, const double radius);
double coneVolume(const double height, const double r1, const double r2);
double capsuleVolume(const double height, const double radius);

core::Vector3f transformVector3f(const core::Vector3f& v, const core::Matrix4f& transformation);
Vector3ds getPointsInSphere(const size_t nbPoints, const double innerRadius);

double frac(const double x);
core::Vector3d frac(const core::Vector3d x);
double mix(const double x, const double y, const double a);
double hash(const double n);
double noise(const core::Vector3d& x);
core::Vector3d mod(const core::Vector3d& v, const int m);
double cells(const core::Vector3d& p, const double cellCount);
double worleyNoise(const core::Vector3d& p, const double cellCount);

size_t getMaterialIdFromOrientation(const core::Vector3d& orientation);

/**
 * @brief Return a random double between 0 and 1
 *
 * @return double A random double between 0 and 1
 */
double rnd0();

/**
 * @brief Return a random double between -0.5 and 0.5
 *
 * @return double A random double between -0.5 and 0.5
 */
double rnd1();

/**
 * @brief Return a predefined random double between -0.5 and 0.5
 *
 * @param index Index of the random double in a predefined array
 * @return double A random double between -0.5 and 0.5
 */
double rnd2(const uint64_t index);

/**
 * @brief Return a controlled random double between -0.5 and 0.5, currently
 * a sinusoidal function
 *
 * @param index Index of the random double in a sinusoidal function
 * @return double A random double between -0.5 and 0.5
 */
double rnd3(const uint64_t index);

/**
 * @brief Randomly alters a quaternion according to the specified parameters
 *
 * @param q Initial quaternion
 * @param seed Random seed
 * @param index Index of the quaternion (typically the index of the
 * corresponding element instance)
 * @param weight Weight of the alteration
 * @return Quaterniond Resulting modified quaternion
 */
core::Quaterniond weightedRandomRotation(const core::Quaterniond& q, const uint64_t seed, const uint64_t index,
                                         const double weight);

/**
 * @brief Generate a random quaternion
 *
 * @param seed Seed to apply to the randomness
 * @return Quaterniond Random quaternion
 */
core::Quaterniond randomQuaternion(const uint64_t seed);

/**
 * @brief Check is test is part of value using the AND operator
 *
 * @param value Value to test
 * @param test Test value
 * @return true is test is part of value using the AND operator, false otherwise
 */
bool andCheck(const uint32_t value, const uint32_t test);

/**
 * @brief Return a Yes/No string representation of a boolean
 *
 * @param value Boolean value
 * @return std::string Yes if true, No otherwise
 */
std::string boolAsString(const bool value);

/**
 * @brief Returns the value of an array of double at a given index. Default
 * value if index is out of bounds
 *
 * @param array Array of doubles
 * @param index Index in the array
 * @param defaultValue Default value if index is out of bounds
 * @return double
 */
double valueFromDoubles(const doubles& array, const size_t index, const double defaultValue);

/**
 * @brief Align a 3D position to a given grid
 *
 * @param gridSize Grid size
 * @param position 3D position
 * @return Vector3d An 3D position aligned to the grid
 */
core::Vector3d getAlignmentToGrid(const double gridSize, const core::Vector3d& position);

/**
 * @brief Fills a cone with spheres that progressively decrease in radius.
 *
 * This function generates spheres that fit perfectly within the profile of a cone defined by two points and two radii.
 * The spheres overlap by half of their radius for an optimal fit.
 *
 * @param center Center of the base of the cone.
 * @param apex Apex of the cone
 * @param constantRadius Uses constant sphere radius if different from 0
 * @return A vector of spheres, each defined by its center and radius.
 */
core::Vector4fs fillConeWithSpheres(const core::Vector4f& center, const core::Vector4f& apex,
                                    const float constantRadius = 0.f);

} // namespace common
} // namespace bioexplorer
