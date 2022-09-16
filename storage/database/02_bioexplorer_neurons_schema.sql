-- The Blue Brain BioExplorer is a tool for scientists to extract and analyse
-- scientific data from visualization
--
-- Copyright 2020-2022 Blue BrainProject / EPFL
--
-- This program is free software: you can redistribute it and/or modify it under
-- the terms of the GNU General Public License as published by the Free Software
-- Foundation, either version 3 of the License, or (at your option) any later
-- version.
--
-- This program is distributed in the hope that it will be useful, but WITHOUT
-- ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
-- FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
-- details.
--
-- You should have received a copy of the GNU General Public License along with
-- this program.  If not, see <https://www.gnu.org/licenses/>.

create table if not exists neurons.configuration
(
    guid  varchar not null
        constraint configuration_pk
            primary key,
    value varchar not null
);

create unique index if not exists configuration_guid_uindex
    on neurons.configuration (guid);

create table if not exists neurons.electrical_type
(
    guid        integer not null
        constraint electrical_type_pk
            primary key,
    code        varchar not null,
    description varchar
);

create unique index if not exists electrical_type_code_uindex
    on neurons.electrical_type (code);

create unique index if not exists electrical_type_guid_uindex
    on neurons.electrical_type (guid);

create table if not exists neurons.layer
(
    guid        integer not null
        constraint layer_pk
            primary key,
    code        varchar not null,
    description varchar
);

create unique index if not exists layer_code_uindex
    on neurons.layer (code);

create unique index if not exists layer_guid_uindex
    on neurons.layer (guid);

create table if not exists neurons.model_template
(
    guid        integer not null
        constraint model_template_pk
            primary key,
    code        varchar not null,
    description varchar
);

create unique index if not exists model_template_guid_uindex
    on neurons.model_template (guid);

create table if not exists neurons.model_type
(
    guid        integer not null
        constraint model_type_pk
            primary key,
    code        varchar not null,
    description integer
);

create unique index if not exists model_type_guid_uindex
    on neurons.model_type (guid);

create table if not exists neurons.morphological_type
(
    guid        integer not null
        constraint morphological_type_pk
            primary key,
    code        varchar not null,
    description varchar
);

create unique index if not exists morphological_type_code_uindex
    on neurons.morphological_type (code);

create unique index if not exists morphological_type_guid_uindex
    on neurons.morphological_type (guid);

create table if not exists neurons.morphology
(
    guid     integer not null
        constraint morphology_pk
            primary key,
    code varchar not null
);

create unique index if not exists morphology_code_uindex
    on neurons.morphology (code);

create unique index if not exists morphology_guid_uindex
    on neurons.morphology (guid);

create table if not exists neurons.morphology_class
(
    guid        integer not null
        constraint morphology_class_pk
            primary key,
    code        varchar not null,
    description varchar
);

create unique index if not exists morphology_class_guid_uindex
    on neurons.morphology_class (guid);

create table if not exists neurons.morphology_type
(
    guid        integer not null
        constraint morphology_type_pk
            primary key,
    code        varchar not null,
    description varchar
);

create unique index if not exists morphology_type_code_uindex
    on neurons.morphology_type (code);

create unique index if not exists morphology_type_guid_uindex
    on neurons.morphology_type (guid);

create table if not exists neurons.node
(
    guid                    integer                    not null
        constraint node_pk
            primary key,
    x                       double precision           not null,
    y                       double precision           not null,
    z                       double precision           not null,
    rotation_x              double precision default 0 not null,
    rotation_y              double precision default 0 not null,
    rotation_z              double precision default 0 not null,
    rotation_w              double precision default 1 not null,
    morphology_guid         integer                    not null,
    morphology_class_guid   integer                    not null,
    electrical_type_guid    integer                    not null,
    morphological_type_guid integer                    not null,
    region_guid             integer                    not null,
    layer_guid              integer                    not null,
    synapse_class_guid      integer                    not null
);

create unique index if not exists node_guid_uindex
    on neurons.node (guid);

create table if not exists neurons.node_type
(
    guid        integer not null
        constraint node_type_pk
            primary key,
    code        varchar not null,
    description varchar
);

create unique index if not exists node_type_guid_uindex
    on neurons.node_type (guid);

create table if not exists neurons.population
(
    guid        integer not null
        constraint population_pk
            primary key,
    name        varchar not null,
    description varchar
);

create unique index if not exists population_guid_uindex
    on neurons.population (guid);

create unique index if not exists population_name_uindex
    on neurons.population (name);

create table if not exists neurons.region
(
    guid        integer not null
        constraint region_pk
            primary key,
    code        varchar not null,
    description varchar
);

create unique index if not exists region_code_uindex
    on neurons.region (code);

create unique index if not exists region_guid_uindex
    on neurons.region (guid);

create table if not exists neurons.section_type
(
    guid        integer not null
        constraint section_type_pk
            primary key,
    description varchar not null
);

create table if not exists neurons.section
(
    morphology_guid     integer                    not null,
    section_guid        integer                    not null,
    section_parent_guid integer                    not null,
    section_type_guid   integer                    not null
        constraint section_section_type_guid_fk
            references neurons.section_type,
    points              bytea                      not null,
    x                   double precision default 0 not null,
    y                   double precision default 0 not null,
    z                   double precision default 0 not null,
    constraint section_pk
        primary key (morphology_guid, section_guid, section_parent_guid)
);

create index if not exists section_morphology_guid_index
    on neurons.section (morphology_guid);

create unique index if not exists section_morphology_guid_section_guid_uindex
    on neurons.section (morphology_guid, section_guid);

create unique index if not exists section_type_description_uindex
    on neurons.section_type (description);

create unique index if not exists section_type_guid_uindex
    on neurons.section_type (guid);

create table if not exists neurons.synapse
(
    synapse_class_guid        integer          not null,
    presynaptic_neuron_guid   integer          not null,
    presynaptic_section_guid  integer          not null,
    presynaptic_segment_guid  integer          not null,
    postsynaptic_neuron_guid  integer          not null,
    postsynaptic_section_guid integer          not null,
    postsynaptic_segment_guid integer          not null,
    surface_x_position       double precision not null,
    surface_y_position       double precision not null,
    surface_z_position       double precision not null,
    center_x_position        double precision not null,
    center_y_position        double precision not null,
    center_z_position        double precision not null
);

create index if not exists synapse_presynaptic_neuron_guid_index
    on neurons.synapse (presynaptic_neuron_guid);

create index if not exists synapse_postsynaptic_neuron_guid_index
    on neurons.synapse (postsynaptic_neuron_guid);

create table if not exists neurons.synapse_class
(
    guid        integer not null
        constraint synapse_class_pk
            primary key,
    code        varchar not null,
    description varchar
);

create unique index if not exists synapse_class_guid_uindex
    on neurons.synapse_class (guid);

create table if not exists neurons.synapse_connectivity
(
    guid              integer not null,
    presynaptic_guid  integer not null,
    postsynaptic_guid integer not null
        constraint synapse_connectivity_pk
            primary key
);

create unique index if not exists synapse_connectivity_guid_uindex
    on neurons.synapse_connectivity (guid);

create table if not exists neurons.target
(
    guid        integer not null
        constraint target_pk
            primary key,
    code        varchar not null,
    description varchar
);

create unique index if not exists target_code_uindex
    on neurons.target (code);

create unique index if not exists target_guid_uindex
    on neurons.target (guid);

create table if not exists neurons.target_node
(
    target_guid integer not null,
    node_guid   integer not null
);

create index if not exists target_node_node_guid_index
    on neurons.target_node (node_guid);

create index if not exists target_node_target_guid_index
    on neurons.target_node (target_guid);

create table if not exists neurons.report_type
(
    guid        integer not null
        constraint report_type_pk
            primary key,
    description varchar not null
);

create unique index if not exists report_type_guid_uindex
    on neurons.report_type (guid);

create table if not exists neurons.report
(
    guid        integer                    not null
        constraint report_pk
            primary key,
    type_guid   integer                    not null
        constraint report_report_type_guid_fk
            references neurons.report_type
            on update cascade on delete cascade,
    description varchar                    not null,
    start_time  double precision default 0 not null,
    end_time    double precision default 0 not null,
    time_step   double precision default 0 not null,
    data_units  varchar,
    time_units  varchar,
    notes       varchar
);

create unique index if not exists report_guid_uindex
    on neurons.report (guid);

create table if not exists neurons.soma_report
(
    guid        integer not null
        constraint soma_report_pk
            primary key,
    report_guid integer not null,
    node_guid   integer not null
        constraint soma_report_node_guid_fk
            references neurons.node
            on update cascade on delete cascade,
    values      bytea   not null
);

create unique index if not exists soma_report_guid_uindex
    on neurons.soma_report (guid);

create table if not exists neurons.spike_report
(
    guid        integer          not null
        constraint spike_report_pk
            primary key,
    report_guid integer          not null,
    node_guid   integer          not null
        constraint spike_report_node_guid_fk
            references neurons.node
            on update cascade on delete cascade,
    timestamp   double precision not null
);

create unique index if not exists spike_report_guid_uindex
    on neurons.spike_report (guid);

create table neurons.compartment_report
(
    report_guid      integer not null,
    node_guid        integer not null
        constraint compartment_report_node_guid_fk
            references neurons.node
            on update cascade on delete cascade,
    section_guid     integer not null,
    compartment_guid integer not null,
    values           bytea   not null,
    constraint compartment_report_pk
        primary key (report_guid, node_guid, section_guid, compartment_guid)
);

create index compartment_report_node_guid_index
    on neurons.compartment_report (node_guid);

create index compartment_report_report_guid_index
    on neurons.compartment_report (report_guid);

create index compartment_report_section_guid_index
    on neurons.compartment_report (section_guid);

create index compartment_report_compartment_guid_index
    on neurons.compartment_report (compartment_guid);
