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

#include <jsonSerialization.h>

#include <tests/paths.h>

#include "ClientServer.h"

#include <fstream>

const std::string ADD_MODEL("add-model");

TEST_CASE_FIXTURE(ClientServer, "missing_params")
{
    try
    {
        makeRequest<core::ModelParams, core::ModelDescriptor>(ADD_MODEL, {});
        REQUIRE(false);
    }
    catch (const rockets::jsonrpc::response_error& e)
    {
        CHECK_EQ(e.code, core::ERROR_ID_MISSING_PARAMS);
        CHECK(e.data.empty());
    }
}

TEST_CASE_FIXTURE(ClientServer, "nonexistant_file")
{
    try
    {
        makeRequest<core::ModelParams, core::ModelDescriptor>(ADD_MODEL, {"wrong", "wrong.xyz"});
        REQUIRE(false);
    }
    catch (const rockets::jsonrpc::response_error& e)
    {
        CHECK_EQ(e.code, core::ERROR_ID_LOADING_BINARY_FAILED);
        CHECK(e.data.empty());
    }
}

TEST_CASE_FIXTURE(ClientServer, "unsupported_type")
{
    try
    {
        makeRequest<core::ModelParams, core::ModelDescriptor>(ADD_MODEL,
                                                              {"unsupported", BRAYNS_TESTDATA_MODEL_UNSUPPORTED_PATH});
        REQUIRE(false);
    }
    catch (const rockets::jsonrpc::response_error& e)
    {
        CHECK_EQ(e.code, core::ERROR_ID_UNSUPPORTED_TYPE);
        REQUIRE(e.data.empty());
    }
}

TEST_CASE_FIXTURE(ClientServer, "xyz")
{
    const auto numModels = getScene().getNumModels();
    CHECK_NOTHROW(
        (makeRequest<core::ModelParams, core::ModelDescriptor>(ADD_MODEL,
                                                               {"monkey", BRAYNS_TESTDATA_MODEL_MONKEY_PATH})));
    CHECK_EQ(getScene().getNumModels(), numModels + 1);
}

TEST_CASE_FIXTURE(ClientServer, "obj")
{
    const auto numModels = getScene().getNumModels();
    core::ModelParams params{"bennu", BRAYNS_TESTDATA_MODEL_BENNU_PATH};
    params.setVisible(false);
    auto model = makeRequest<core::ModelParams, core::ModelDescriptor>(ADD_MODEL, params);
    CHECK_EQ(getScene().getNumModels(), numModels + 1);
    CHECK_EQ(model.getName(), params.getName());
    CHECK_EQ(model.getPath(), params.getPath());
    CHECK(!model.getVisible());
}

TEST_CASE_FIXTURE(ClientServer, "xyz_obj")
{
    const auto initialNbModels = getScene().getNumModels();
    CHECK_NOTHROW(
        (makeRequest<core::ModelParams, core::ModelDescriptor>(ADD_MODEL,
                                                               {"monkey", BRAYNS_TESTDATA_MODEL_MONKEY_PATH})));
    CHECK_NOTHROW((
        makeRequest<core::ModelParams, core::ModelDescriptor>(ADD_MODEL, {"bennu", BRAYNS_TESTDATA_MODEL_BENNU_PATH})));
    auto newNbModels = getScene().getNumModels();
    CHECK_EQ(initialNbModels + 2, newNbModels);
    getScene().removeModel(newNbModels - 1);
    newNbModels = getScene().getNumModels();
    CHECK_EQ(initialNbModels + 1, newNbModels);
}

TEST_CASE_FIXTURE(ClientServer, "broken_xyz")
{
    try
    {
        makeRequest<core::ModelParams, core::ModelDescriptor>(ADD_MODEL, {"broken", BRAYNS_TESTDATA_MODEL_BROKEN_PATH});
        REQUIRE(false);
    }
    catch (const rockets::jsonrpc::response_error& e)
    {
        CHECK_EQ(e.code, core::ERROR_ID_LOADING_BINARY_FAILED);
        CHECK(std::string(e.what()) == "Invalid content in line 1: 2.500000 3.437500");
    }
}

#if PLATFORM_USE_LIBARCHIVE
TEST_CASE_FIXTURE(ClientServer, "obj_zip")
{
    const auto numModels = getScene().getNumModels();
    core::ModelParams params{"bennu", BRAYNS_TESTDATA_MODEL_BENNU_PATH};
    auto model = makeRequest<core::ModelParams, core::ModelDescriptor>(ADD_MODEL, params);
    CHECK_EQ(getScene().getNumModels(), numModels + 1);
    CHECK_EQ(model.getName(), params.getName());
    CHECK_EQ(model.getPath(), params.getPath());
}
#endif

TEST_CASE_FIXTURE(ClientServer, "mesh_loader_properties_valid")
{
    const auto numModels = getScene().getNumModels();
    core::PropertyMap properties;
    properties.setProperty({"geometryQuality", std::string("low")});
    properties.setProperty({"unused", 42});
    core::ModelParams params{"bennu", BRAYNS_TESTDATA_MODEL_BENNU_PATH, properties};

    auto model = makeRequest<core::ModelParams, core::ModelDescriptor>(ADD_MODEL, params);
    CHECK_EQ(getScene().getNumModels(), numModels + 1);
    CHECK_EQ(model.getName(), params.getName());
    CHECK_EQ(model.getPath(), params.getPath());
}

TEST_CASE_FIXTURE(ClientServer, "mesh_loader_properties_invalid")
{
    core::PropertyMap properties;
    properties.setProperty({"geometryQuality", std::string("INVALID")});
    core::ModelParams params{"bennu", BRAYNS_TESTDATA_MODEL_BENNU_PATH, properties};

    try
    {
        makeRequest<core::ModelParams, core::ModelDescriptor>(ADD_MODEL, params);
        REQUIRE(false);
    }
    catch (std::runtime_error& e)
    {
        CHECK(std::string(e.what()) == "Could not match enum 'INVALID'");
    }
}

TEST_CASE_FIXTURE(ClientServer, "protein_loader")
{
    core::PropertyMap properties;
    properties.setProperty({"radiusMultiplier", 2.5});
    properties.setProperty({"colorScheme",
                            core::enumToString(core::ProteinColorScheme::protein_chains),
                            core::enumNames<core::ProteinColorScheme>(),
                            {}});

    const auto numModels = getScene().getNumModels();

    core::ModelParams params{"1mbs", BRAYNS_TESTDATA_MODEL_PDB_PATH, properties};

    auto model = makeRequest<core::ModelParams, core::ModelDescriptor>(ADD_MODEL, params);
    CHECK_EQ(getScene().getNumModels(), numModels + 1);
    CHECK_EQ(model.getName(), params.getName());
    CHECK_EQ(model.getPath(), params.getPath());
    CHECK(!model.getBounds().isEmpty());
}

TEST_CASE_FIXTURE(ClientServer, "cancel")
{
    auto request =
        getJsonRpcClient().request<core::ModelParams, core::ModelDescriptor>(ADD_MODEL, {"forever", "forever"});

    request.cancel();

    while (!request.is_ready())
        process();

    CHECK_THROWS_AS(request.get(), std::runtime_error);
}

TEST_CASE_FIXTURE(ClientServer, "close_client_while_pending_request")
{
    auto wsClient = std::make_unique<rockets::ws::Client>();

    connect(*wsClient);

    auto responseFuture =
        rockets::jsonrpc::Client<rockets::ws::Client>{*wsClient}.request<core::ModelParams, core::ModelDescriptor>(
            ADD_MODEL, {"monkey", BRAYNS_TESTDATA_MODEL_MONKEY_PATH});

    auto asyncWait = std::async(std::launch::async,
                                [&responseFuture, &wsClient, this]
                                {
                                    wsClient->process(10);
                                    process();

                                    wsClient.reset(); // close client connection
                                    process();

                                    responseFuture.get();
                                });

    CHECK_THROWS_AS(asyncWait.get(), rockets::jsonrpc::response_error);
}

TEST_CASE_FIXTURE(ClientServer, "folder")
{
    CHECK_THROWS_AS((makeRequest<core::ModelParams, core::ModelDescriptorPtr>(
                        ADD_MODEL, {"folder", BRAYNS_TESTDATA_VALID_MODELS_PATH})),
                    rockets::jsonrpc::response_error);
}
