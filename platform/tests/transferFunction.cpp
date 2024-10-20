/*
    Copyright 2018 - 2024 Blue Brain Project / EPFL

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

#include <platform/core/Core.h>

#ifdef PLATFORM_USE_OSPRAY
#include <ospray/SDK/common/Data.h>
#include <ospray/SDK/common/Managed.h>
#include <platform/engines/ospray/OSPRayModel.h>
#endif

#include "ClientServer.h"
#include "PDiffHelpers.h"

const std::string SET_TF("set-model-transfer-function");
const std::string GET_TF("get-model-transfer-function");

TEST_CASE_FIXTURE(ClientServer, "set_transfer_function")
{
    core::TransferFunction tf;
    tf.setValuesRange({0, 10});
    tf.setColorMap({"test", {{1, 1, 1}, {0, 1, 0}, {0.1, 1, 0.5}}});
    tf.setControlPoints({{0, 0.1}, {0.2, 1.}, {1., 0.8}});

    makeNotification<core::ModelTransferFunction>(SET_TF, {0, tf});

    const auto& newTF = getScene().getModel(0)->getModel().getTransferFunction();
    CHECK(tf.getColorMap() == newTF.getColorMap());
    CHECK(tf.getControlPoints() == newTF.getControlPoints());
    CHECK_EQ(tf.getValuesRange(), newTF.getValuesRange());
}

TEST_CASE_FIXTURE(ClientServer, "set_transfer_function_invalid_model")
{
    core::TransferFunction tf;

    CHECK_THROWS_AS((makeRequest<core::ModelTransferFunction, bool>(SET_TF, {42, tf})), std::runtime_error);
}

TEST_CASE_FIXTURE(ClientServer, "get_transfer_function")
{
    const auto& tf = getScene().getModel(0)->getModel().getTransferFunction();

    const auto rpcTF = makeRequest<core::ObjectID, core::TransferFunction>(GET_TF, {0});

    CHECK(tf.getColorMap() == rpcTF.getColorMap());
    CHECK(tf.getControlPoints() == rpcTF.getControlPoints());
    CHECK_EQ(tf.getValuesRange(), rpcTF.getValuesRange());
}

TEST_CASE_FIXTURE(ClientServer, "get_transfer_function_invalid_model")
{
    CHECK_THROWS_AS((makeRequest<core::ObjectID, core::TransferFunction>(GET_TF, {42})), std::runtime_error);
}

#ifdef PLATFORM_USE_OSPRAY
TEST_CASE_FIXTURE(ClientServer, "validate_opacity_interpolation")
{
    auto& model = getScene().getModel(0)->getModel();
    auto& tf = model.getTransferFunction();
    tf.clear();

    commitAndRender();

    auto& ospModel = static_cast<core::OSPRayModel&>(model);
    auto ospTF = reinterpret_cast<ospray::ManagedObject*>(ospModel.transferFunction());
    auto colors = ospTF->getParamData("colors", nullptr);
    REQUIRE_EQ(colors->size(), 2);
    CHECK_EQ(core::Vector3f(((float*)colors->data)[0]), core::Vector3f(tf.getColors()[0]));
    CHECK_EQ(core::Vector3f(((float*)colors->data)[3]), core::Vector3f(tf.getColors()[1]));

    auto opacities = ospTF->getParamData("opacities", nullptr);
    REQUIRE_EQ(opacities->size(), 256);
    for (size_t i = 0; i < 256; ++i)
        CHECK_EQ(((float*)opacities->data)[i], i / 255.f);
}
#endif
