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

#include <pqxx/pqxx>

namespace bioexplorer
{
namespace io
{
namespace db
{
using ConnectionPtr = std::shared_ptr<pqxx::connection>;

/**
 * @brief The DBConnector class allows the BioExplorer to communicate with a
 * PostgreSQL database. The DBConnector requires the pqxx library to be found at
 * compilation time.
 *
 */
class DBConnector
{
public:
    /**
     * @brief Get the Instance object
     *
     * @return GeneralSettings* Pointer to the object
     */
    static DBConnector& getInstance()
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (!_instance)
            _instance = new DBConnector();
        return *_instance;
    }

    /**
     * @brief Connects to the database using the provided command line arguments
     *
     * @param arguments Command line arguments
     */
    void init(const CommandLineArguments& arguments);

    /**
     * @brief Get the number of connections to the database
     *
     * @return size_t Number of connections to the database
     */
    size_t getNbConnections() const { return _dbNbConnections; }

    /**
     * @brief Get the maximum number of records returned by a DB query
     *
     * @return size_t The maximum number of records returned by a DB query
     */
    size_t getBatchSize() const { return _dbBatchSize; }

    /**
     * @brief Remove all bricks from the PostgreSQL database
     *
     */
    void clearBricks();

    /**
     * @brief Get the Scene configuration
     *
     * @return Configuration of the out-of-code read-only scene
     */
    const details::OOCSceneConfigurationDetails getSceneConfiguration();

    /**
     * @brief Get the Brick object
     *
     * @param brickId Identifier of the brick
     * @param version Version of the binary buffer with the contents of the
     * brick
     * @param nbModels The number of models contained in the brick
     * @return std::stringstream The binary buffer with the contents of the
     * brick
     */
    std::stringstream getBrick(const int32_t brickId, const uint32_t& version, uint32_t& nbModels);

    /**
     * @brief Inserts a brick into the PostgreSQL database
     *
     * @param brickId Identifier of the brick
     * @param version Version of the binary buffer with the contents of the
     * brick
     * @param nbModels The number of models contained in the brick
     * @param buffer The binary buffer with the contents of the brick
     */
    void insertBrick(const int32_t brickId, const uint32_t version, const uint32_t nbModels,
                     const std::stringstream& buffer);

    /**
     * @brief Get the Nodes for a given population
     *
     * @param populationName Name of the population
     * @param sqlCondition SQL condition
     * @return GeometryNodes Vasculature nodes
     */
    common::GeometryNodes getVasculatureNodes(const std::string& populationName, const std::string& sqlCondition = "",
                                              const std::string& limits = "") const;

    /**
     * @brief Get the sections for a given population
     *
     * @param populationName Name of the population
     * @param sqlCondition SQL condition
     * @return Section ids
     */
    uint64_ts getVasculatureSections(const std::string& populationName, const std::string& sqlCondition = "");

    /**
     * @brief Get the number of nodes for a given population and a given filter
     *
     * @param populationName Name of the population
     * @param sqlCondition SQL condition
     * @return Number of nodes
     */
    uint64_t getVasculatureNbNodes(const std::string& populationName, const std::string& sqlCondition);

    /**
     * @brief Get the Vasculature radius range
     *
     * @param populationName Name of the population
     * @param sqlCondition SQL condition
     * @return Vector2d Min and max radius for the node selection
     */
    core::Vector2d getVasculatureRadiusRange(const std::string& populationName, const std::string& sqlCondition) const;

    /**
     * @brief Get the Edges for a given population
     *
     * @param populationName Name of the population
     * @return EdgeNodes
     */
    common::GeometryEdges getVasculatureEdges(const std::string& populationName,
                                              const std::string& sqlCondition = "") const;

    /**
     * @brief Get the bifurcations for a given population
     *
     * @param populationName Name of the population
     * @return Bifurcations
     */
    common::Bifurcations getVasculatureBifurcations(const std::string& populationName) const;

    /**
     * @brief Get information about the simulation Report
     *
     * @param populationName Name of the population
     * @param simulationReportId Simulation report identifier
     * @return SimulationReport Information about the simulation Report
     */

    common::SimulationReport getSimulationReport(const std::string& populationName,
                                                 const int32_t simulationReportId) const;

    /**
     * @brief Get time series from simulation report
     *
     * @param populationName Name of the population
     * @param simulationReportId Simulation report identifier
     * @param frame Frame number
     * @return floats Values of the simulation frame
     */
    floats getVasculatureSimulationTimeSeries(const std::string& populationName, const int32_t simulationReportId,
                                              const int32_t frame) const;

    /**
     * @brief Get the astrocytes locations
     *
     * @param populationName Name of the population
     * @param sqlCondition String containing an WHERE condition for the SQL
     * statement
     * @return SomaMap A map of somas (position, radius, etc)
     */
    morphology::AstrocyteSomaMap getAstrocytes(const std::string& populationName,
                                               const std::string& sqlCondition = "") const;

    /**
     * @brief Get the sections of a given astrocyte
     *
     * @param populationName Name of the population
     * @param astrocyteId Identifier of the astrocyte
     * @param connectedToSomaOnly Loads only sections that are directly
     * connected to the soma if set to true
     * @return SectionMap A map of sections
     */
    morphology::SectionMap getAstrocyteSections(const std::string& populationName, const int64_t astrocyteId,
                                                const bool connectedToSomaOnly) const;

    /**
     * @brief Get the end-feet as nodes for a given astrocyte
     *
     * @param vasculaturePopulationName Name of the vasculature population
     * @param connectomePopulationName Name of the connectome population
     * @param astrocyteId Identifier of the astrocyte
     * @return EndFootNodesMap A map of end-feet
     */
    morphology::EndFootMap getAstrocyteEndFeet(const std::string& vasculaturePopulationName,
                                               const std::string& connectomePopulationName,
                                               const uint64_t astrocyteId) const;

    /**
     * @brief Get the micro-domain for a given astrocyte
     *
     * @param populationName Name of the population
     * @param astrocyteId Identifier of the astrocyte
     * @return Micro-domain triangle mesh
     */
    core::TriangleMesh getAstrocyteMicroDomain(const std::string& populationName, const uint64_t astrocyteId) const;

    /**
     * @brief Get the Neuron Circuit Configuration details
     *
     * @param populationName Name of the population
     * @return StringMap Map of key value strings
     */
    StringMap getNeuronConfiguration(const std::string& populationName) const;

    /**
     * @brief Get the number of neurons for a given population and a specific filter
     *
     * @param populationName Name of the population
     * @param sqlCondition String containing an WHERE condition for the SQL
     * statement
     * @return Number of neurons
     */
    uint64_t getNumberOfNeurons(const std::string& populationName, const std::string& sqlCondition = "") const;

    /**
     * @brief Get the neurons locations
     *
     * @param populationName Name of the population
     * @param sqlCondition String containing an WHERE condition for the SQL
     * statement
     * @return NeuronSomaMap A map of neurons (position, type, etc)
     */
    morphology::NeuronSomaMap getNeurons(const std::string& populationName, const std::string& sqlCondition = "") const;

    /**
     * @brief Get the relative path to the morphology file for a given neuron Id
     *
     * @param populationName Name of the population
     * @param neuronId Identifier of the neuron
     * @return std::string Relative path to the morphology file
     */
    std::string getNeuronMorphologyRelativePath(const std::string& populationName, const uint64_t neuronId) const;

    /**
     * @brief Get the sections of a given neuron
     *
     * @param populationName Name of the population
     * @param neuronId Identifier of the neuron
     * @param sqlCondition String containing an WHERE condition for the SQL
     * statement
     * @return SectionMap A map of sections
     */
    morphology::SectionMap getNeuronSections(const std::string& populationName, const uint64_t neuronId,
                                             const std::string& sqlCondition = "") const;

    /**
     * @brief Get the afferent synapses attached to a given neuron
     *
     * @param populationName Name of the population
     * @param neuronId Identifier of the neuron
     * @param sqlCondition String containing an WHERE condition for the SQL
     * statement
     * @return SectionSynapseMap A map of synapses
     */
    morphology::SectionSynapseMap getNeuronAfferentSynapses(const std::string& populationName, const uint64_t neuronId,
                                                            const std::string& sqlCondition = "") const;

    /**
     * @brief Get a selection of spikes from a neuron spike report
     *
     * @param populationName Name of the population
     * @param reportId Simulation report identifier
     * @param endTime End time of the selection
     * @return uint64_ts Spiking neuron ids for the specified time selection
     */
    morphology::SpikesMap getNeuronSpikeReportValues(const std::string& populationName, const uint64_t reportId,
                                                     const double startTime, const double endTime) const;

    /**
     * @brief Get the Neuron Soma Report Guids
     *
     * @param populationName Name of the population
     * @param reportId Simulation report identifier
     * @return uint64_tm Neuron Soma Report Guids
     */
    uint64_tm getSimulatedNodesGuids(const std::string& populationName, const uint64_t reportId) const;

    /**
     * @brief Get the Neuron soma simulation values
     *
     * @param populationName Name of the population
     * @param reportId Simulation report identifier
     * @param frame Simulation frame
     * @return floats The Neuron soma simulation values
     */
    void getNeuronSomaReportValues(const std::string& populationName, const uint64_t reportId, const uint64_t frame,
                                   floats& values) const;

    /**
     * @brief Get the neuron section compartments for a given simulation report
     *
     * @param populationName Name of the population
     * @param reportId Simulation report identifier
     * @param neuronId Neuron identifier
     * @param neuronId Section identifier
     * @return uint64_ts Compartments identifiers
     */
    uint64_ts getNeuronSectionCompartments(const std::string& populationName, const uint64_t reportId,
                                           const uint64_t nodeId, const uint64_t sectionId) const;

    /**
     * @brief Get the Neuron compartment simulation values
     *
     * @param populationName Name of the population
     * @param reportId Simulation report identifier
     * @param frame Simulation frame
     * @return floats The Neuron compartment simulation values
     */
    floats getNeuronCompartmentReportValues(const std::string& populationName, const uint64_t reportId,
                                            const uint64_t frame) const;

    /**
     * @brief Get the regions from the brain atlas
     *
     * @param populationName Name of the population
     * @param sqlCondition String containing an WHERE condition for the SQL
     * statement
     * @return A vector of regions Ids
     */
    uint64_ts getAtlasRegions(const std::string& populationName, const std::string& sqlCondition = "") const;

    /**
     * @brief Get the cells from the brain atlas
     *
     * @param populationName Name of the population
     * @param regionId Region identifier
     * @param sqlCondition String containing an WHERE condition for the SQL
     * statement
     * @return CellMap A map of cells (position, orientation, type, etc)
     */
    morphology::CellMap getAtlasCells(const std::string& populationName, const uint64_t regionId,
                                      const std::string& sqlCondition = "") const;

    /**
     * @brief Get the mesh of a given region from the brain atlas
     *
     * @param populationName Name of the population
     * @param regionId Region identifier
     * statement
     * @return TrianglesMesh A triangles mesh
     */
    core::TriangleMesh getAtlasMesh(const std::string& populationName, const uint64_t regionId) const;

    /**
     * @brief Get the White Matter streamlines for a given population
     *
     * @param populationName Name of the population
     * @param sqlCondition SQL condition
     * @return WhiteMatterStreamlines White matter streamlines
     */
    connectomics::WhiteMatterStreamlines getWhiteMatterStreamlines(const std::string& populationName,
                                                                   const std::string& sqlCondition = "") const;

    /**
     * @brief Get positions of synapses
     *
     * @param populationName Name of the population
     * @param sqlCondition SQL condition
     * @return Map of synapses indexed by ID
     */
    morphology::SynapsesMap getSynapses(const std::string& populationName, const std::string& sqlCondition = "") const;

    /**
     * @brief Get positions of synapses for efficacy report
     *
     * @param populationName Name of the population
     * @param sqlCondition SQL condition
     * @return Positions of synapses for efficacy report
     */
    Vector3ds getSynapseEfficacyPositions(const std::string& populationName,
                                          const std::string& sqlCondition = "") const;

    /**
     * @brief Get synapse efficacy report values
     *
     * @param populationName Name of the population
     * @param frame Simulation frame
     * @param sqlCondition SQL condition
     * @return Values of synapses efficacy
     */
    std::map<uint64_t, floats> getSynapseEfficacyReportValues(const std::string& populationName, const uint64_t frame,
                                                              const std::string& sqlCondition = "") const;

    /**
     * @brief Get synaptome
     *
     * @param populationName Name of the population
     * @param sqlCondition SQL condition
     * @return Map of pre and postsynaptic node guids
     */
    uint64_tm getSynaptome(const std::string& populationName, const std::string& sqlCondition = "") const;

    static std::mutex _mutex;
    static DBConnector* _instance;

private:
    DBConnector();
    ~DBConnector();

    size_t _dbNbConnections{DEFAULT_DB_NB_CONNECTIONS};
    size_t _dbBatchSize{DEFAULT_BATCH_SIZE};

    std::string _connectionString;

    std::vector<ConnectionPtr> _connections;
    bool _initialized{false};
};

} // namespace db
} // namespace io
} // namespace bioexplorer
