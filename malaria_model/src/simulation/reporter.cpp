#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <queue>
#include <map>

#include <cassert>

#include <chrono>
#include <thread>

#include "common/common.h"
#include "util/util.h"

#include "village/village.h"
#include "village/mosquito_dispersion.h"
#include "human/human.h"
#include "human/blood_system.h"

#include "simulation/reporter.h"



namespace simulation{

ReportManager::ReportManager(

        const bool if_batch_mode,

        const std::string s_title,
        const std::string s,
        const int time_max,

        const bool if_output_prevalence_daily,
        const bool if_output_drug_daily,
        const bool if_output_infection_daily,
        const bool if_output_infection_daily_village,
        const bool if_output_mosquito_daily,
        const bool if_output_treatment_daily,
        const bool if_output_mutation_daily,
        const bool if_output_clearance_natural_daily,
        const bool if_output_clearance_drug_daily,
        const bool if_output_village_prev_annual,
        const bool if_output_village_infection_annual,
        const bool if_output_mobility_daily,
        const bool if_output_mobility_daily_od,
        const bool if_output_mosquito_dist_daily,
        const bool if_output_mobility_daily_intensity,


        const int prevalence_daily_y_max,
        int drug_failure_if_clinical_on_day,
        size_t drug_failure_measurement_length,

        village::VillageManager* vm,
        human::HumanManager* hm,
        human::BloodSystemManager* bm,
        village::MosquitoDP* msqdpm

    ) : 
        kIf_batch_mode(if_batch_mode),
        kOutput_title(s_title),
        output_prefix(s),
        sim_time_max(time_max),

        kReport_selector{
            if_output_prevalence_daily,
            if_output_drug_daily,
            if_output_infection_daily,
            if_output_infection_daily_village,
            if_output_mosquito_daily,
            if_output_treatment_daily,
            if_output_mutation_daily,
            if_output_clearance_natural_daily,
            if_output_clearance_drug_daily,
            if_output_village_prev_annual,
            if_output_village_infection_annual,
            if_output_mobility_daily,
            if_output_mobility_daily_od,
            if_output_mosquito_dist_daily,
            if_output_mobility_daily_intensity
        },

        kPrevalence_daily_y_max(prevalence_daily_y_max),
        kDrug_failure_if_clinical_on_day(drug_failure_if_clinical_on_day),
        kDrug_failure_measurement_length(drug_failure_measurement_length),

        vll_mgr(vm),
        hmn_mgr(hm),
        bld_mgr(bm),
        msq_dp_mgr(msqdpm),

        last_known_location_of(
            hmn_mgr->get_at_village_of(),
            hmn_mgr->get_at_village_of()+hmn_mgr->get_sum_num_humans()
            )
    {
            // file_name_list = ;


}

ReportManager::~ReportManager() {
    for (uint ii = 0; ii < this->kReport_selector.size(); ii++) {
        if (this->kReport_selector.at(ii)) {
            std::string fn = this->kReport_file_names.at(ii);
            delete this->filename_stream_map[fn];
        }
    }
}

void ReportManager::open_ofstreams() {
    
    assert(this->kReport_selector.size() == this->kReport_file_names.size());

    for (uint ii = 0; ii < this->kReport_selector.size(); ii++) {
        if (this->kReport_selector.at(ii)) {

            std::string fn = this->kReport_file_names.at(ii);

            this->filename_stream_map[fn] = new std::ofstream(this->output_prefix + fn);
            if (!this->filename_stream_map[fn] || !*this->filename_stream_map[fn]) {
                perror(fn.c_str());
            }
        }
    }

    // for (auto& fn : this->kReport_file_names) {

    //     if(this->kReport_selector)
    //     this->filename_stream_map[fn] = new std::ofstream(this->output_prefix + fn);
    //     if (!this->filename_stream_map[fn] || !*this->filename_stream_map[fn]){
    //         perror(fn.c_str());
    //     }

    // }
}

void ReportManager::close_ofstreams() {
    for (auto& fs_pair : this->filename_stream_map) {
        fs_pair.second->close();
    }

    std::chrono::seconds duration(this->kGnuplot_pause_interval*2);
    std::this_thread::sleep_for(duration);

    if ( !this->kIf_batch_mode ) {
        if(system("killall gnuplot &")){};
    }
}

void ReportManager::end_of_day_reports(int sim_time){

    if (this->kReport_selector.at(9)) {
        if (sim_time % common::kNum_days_in_one_year == 0) {
            this->annual_report_village_prev(
                sim_time,
                this->vll_mgr,
                this->filename_stream_map[this->kReport_file_names[9]]
            );
        }
    }

    if (this->kReport_selector.at(10)) {
        // if (sim_time % 365 == 0) {
            this->annual_report_village_infection(
                sim_time,
                this->vll_mgr,
                this->bld_mgr,
                this->hmn_mgr,
                this->filename_stream_map[this->kReport_file_names[10]]
            );
        // }
    }

    if (this->kReport_selector[11]) {
        this->daily_report_mobility(
            sim_time,
            this->hmn_mgr,
            this->filename_stream_map[this->kReport_file_names[11]]
        );
    }

    if (this->kReport_selector[12]) {
        this->daily_report_mobility_od(
            sim_time,
            this->hmn_mgr,
            this->filename_stream_map[this->kReport_file_names[12]]
        );
    }

    if (this->kReport_selector[13]) {
        this->daily_report_mosquito_dist(
            sim_time,
            this->filename_stream_map[this->kReport_file_names[13]]
        );
    }

    if (this->kReport_selector[14]) {
        this->daily_report_mobility_intensity(
            sim_time,
            this->filename_stream_map[this->kReport_file_names[14]]
        );
    }

    if (this->kReport_selector.at(0)) {
        this->daily_report_prevalence(
            sim_time,
            this->bld_mgr,
            this->filename_stream_map["_prevalence_daily.csv"]
        );
    }

    if (this->kReport_selector.at(1)) {
        this->daily_report_drug(
            sim_time,
            this->bld_mgr,
            this->vll_mgr,
            this->filename_stream_map[this->kReport_file_names.at(1)]
        );
    }

    if (this->kReport_selector.at(2)) {
        this->daily_report_infection(
            sim_time,
            this->vll_mgr,
            this->filename_stream_map[this->kReport_file_names.at(2)]
        );
    }

    if (this->kReport_selector.at(3)) {
        this->daily_report_infection_village(
            sim_time,
            this->vll_mgr,
            this->filename_stream_map[this->kReport_file_names.at(3)]
        );
    }

    if (this->kReport_selector.at(4)) {
        this->daily_report_mosquito(
            sim_time,
            this->vll_mgr,
            this->filename_stream_map[this->kReport_file_names.at(4)]
        );
    }

    if (this->kReport_selector.at(5)) {
        this->daily_report_treatment(
            sim_time,
            this->vll_mgr,
            this->bld_mgr,
            // this->hmn_mgr,
            this->filename_stream_map[this->kReport_file_names.at(5)]
        );
    }

    if (this->kReport_selector.at(6)) {
        this->daily_report_mutation(
            sim_time,
            this->filename_stream_map[this->kReport_file_names.at(6)]
        );
    }

    if (this->kReport_selector.at(7)) {
        this->daily_report_clearance_natural(
            sim_time,
            this->filename_stream_map[this->kReport_file_names.at(7)]
        );
    }

    if (this->kReport_selector.at(8)) {
        this->daily_report_clearance_drug(
            sim_time,
            this->filename_stream_map[this->kReport_file_names.at(8)]
        );
    }

    if (sim_time % this->kOstream_flush_interval == 0 && sim_time!=0) {
        for (const auto& fs_pair : this->filename_stream_map) {
            fs_pair.second->flush();
        }
    }

    // this->daily_report_village(sim_time);

    // this->daily_report_on_dominant_type(
    //     sim_time,
    //     this->bld_mgr->get_dominant_type(),
    //     this->hmn_mgr->get_at_village_of(),
    //     this->hmn_mgr->get_age_of(),
    //     this->hmn_mgr->sum_num_humans,
    //     this->vll_mgr->sum_num_villages,
    //     human::kNum_age_bins
    //     );
    // this->daily_report_on_immunity(
    //     sim_time,
    //     this->bld_mgr->get_immunity_level_of(),
    //     this->bld_mgr->get_cummulative_exposures_of(),
    //     this->hmn_mgr->get_at_village_of(),
    //     this->bld_mgr->sum_num_systems,
    //     this->vll_mgr->sum_num_villages
    //     );
    // this->daily_report_on_age_distribution(
    //     sim_time,
    //     this->hmn_mgr->get_age_of(),
    //     this->hmn_mgr->sum_num_humans,
    //     human::kNum_age_bins
    //     );

}

void ReportManager::annual_report_village_infection(
        int sim_time,
        village::VillageManager* v_mgr,
        human::BloodSystemManager* b_mgr,
        human::HumanManager* h_mgr,
        std::ofstream* os
    ) {

    if (sim_time == 0) {
        *os << "day" << this->kCSV_sep
            << "v_id" << this->kCSV_sep
            << "v_hpop" << this->kCSV_sep
            << "v_prev" << this->kCSV_sep
            << "bites_1y" << this->kCSV_sep
            << "infections_1y" << this->kCSV_sep
            << "clinical_cases_1y" << this->kCSV_sep
            << "asymptomatic_cases_1y" << this->kCSV_sep
            << "\n";

        // for (int vv = 0; vv < v_mgr->sum_num_villages; vv++) {
        //     *os << this->kCSV_sep;
        //     *os << "v_{"<< vv <<"}";
        // }
        // for (int vv = 0; vv < v_mgr->sum_num_villages; vv++) {
        //     *os << this->kCSV_sep;
        //     *os << "v_{"<< vv <<"}";
        // }
    }

    for( int vv = 0; vv < v_mgr->sum_num_villages; vv++) {
        v_mgr->daily_record_of_new_bites_annual[vv] += v_mgr->daily_record_of_new_bites[vv];
        v_mgr->daily_record_of_new_infections_annual[vv] += v_mgr->daily_record_of_new_infections[vv];
    }

    for ( int ss = 0; ss < b_mgr->sum_num_systems; ss++) {
        if (b_mgr->report_daily_new_clinical_case[ss]) {
            v_mgr->daily_record_of_new_clinical_cases_annual[h_mgr->get_at_village(ss)]++;
        }
        if (b_mgr->report_daily_new_asymptomatic_case[ss]) {
            v_mgr->daily_record_of_new_asymptomatic_cases_annual[h_mgr->get_at_village(ss)]++;
        }
    }
    

    // int sum = 0;
    // for( int vv = 0; vv < v_mgr->sum_num_villages; vv++) {
    //     sum+=v_mgr->daily_record_of_new_bites_annual[vv];
    // }
    // std::cout << "RM: sum=" << sum << "\n";
    // std::cin.ignore();

    if (sim_time % static_cast<int>(common::kDays_per_year) == 0) {

        for (int vv = 0; vv < v_mgr->sum_num_villages; vv++) {
            *os << sim_time << this->kCSV_sep
                << vv << this->kCSV_sep
                << v_mgr->get_home_population_of_village(vv) << this->kCSV_sep
                << v_mgr->get_prevalence_of(vv) << this->kCSV_sep
                << v_mgr->daily_record_of_new_bites_annual[vv] << this->kCSV_sep
                << v_mgr->daily_record_of_new_infections_annual[vv] << this->kCSV_sep
                << v_mgr->daily_record_of_new_clinical_cases_annual[vv] << this->kCSV_sep
                << v_mgr->daily_record_of_new_asymptomatic_cases_annual[vv]
                << "\n";

            v_mgr->daily_record_of_new_bites_annual[vv] = 0;
            v_mgr->daily_record_of_new_infections_annual[vv] = 0;
            v_mgr->daily_record_of_new_clinical_cases_annual[vv] = 0;
            v_mgr->daily_record_of_new_asymptomatic_cases_annual[vv] = 0;

        }

        // *os << sim_time;
        // for (auto& vv: v_mgr->daily_record_of_new_bites_annual) {
        //     *os << this->kCSV_sep;
        //     *os << vv;

        //     vv = 0;
        // }
        // for (auto& vv: v_mgr->daily_record_of_new_infections_annual) {
        //     *os << this->kCSV_sep;
        //     *os << vv;

        //     vv = 0;
        // }
        // *os << "\n";
        
    }


}

void ReportManager::annual_report_village_prev(
        int sim_time,
        village::VillageManager* v_mgr,
        std::ofstream* os
    ) {

    if (sim_time == 0) {
        *os << "day";
        for (int vv = 0; vv < v_mgr->sum_num_villages; vv++) {
            *os << this->kCSV_sep;
            *os << "v_{"<< vv <<"}";
        }
        *os << "\n";
    }

    *os << sim_time;
    for (int vv = 0; vv < v_mgr->sum_num_villages; vv++) {
        *os << this->kCSV_sep;
        *os << v_mgr->get_prevalence_of(vv);
    }
    *os << "\n";
}

void ReportManager::daily_report_mobility(
        int sim_time,
        const human::HumanManager* h_mgr,
        std::ofstream* os
    ) {

    if (sim_time == 0) {
        *os << "day" << this->kData_sep;
        for (int hh = 0; hh < h_mgr->get_sum_num_humans(); hh++) {
            *os << hh << this->kData_sep;
        }
        *os << "\n";
    }
    *os << sim_time << this->kData_sep;
    for (int hh = 0; hh < h_mgr->get_sum_num_humans(); hh++) {
        *os << h_mgr->get_at_village(hh) << this->kData_sep;
    }
    *os << "\n";

}

void ReportManager::daily_report_mobility_od(
        int sim_time,
        const human::HumanManager* h_mgr,
        std::ofstream* os
    ) {

    if (sim_time == 0) {
        *os << "day"  << this->kData_sep 
            << "orig" << this->kData_sep
            << "dest" << this->kData_sep
            << "hid"  << this->kData_sep
            << "\n";
        for (int hh = 0; hh < h_mgr->get_sum_num_humans(); hh++) {
            this->last_known_location_of[hh] = h_mgr->get_home_village(hh);
        }
    }

    for (int hh = 0; hh < h_mgr->get_sum_num_humans(); hh++) {
        int orig = this->last_known_location_of[hh];
        int dest = h_mgr->get_at_village(hh);
        if (orig != dest) {
            *os << sim_time << this->kData_sep
                << orig << this->kData_sep
                << dest << this->kData_sep
                << hh << this->kData_sep
                << "\n";
            this->last_known_location_of[hh] = dest;
        }
    }

}

void ReportManager::daily_report_mosquito_dist(
        int sim_time,
        std::ofstream* os
    ) {

    if (sim_time == 0) {
        *os << "day"
            << this->kData_sep << "tot_infected"
            << this->kData_sep << "tot_infectious"
            << this->kData_sep << "tot_infected_forest"
            << this->kData_sep << "tot_infectious_forest"
            << this->kData_sep << "pit_infected"
            << this->kData_sep << "pit_infectious";
        for (int vv = 0; vv < this->vll_mgr->sum_num_villages; vv++) {
            *os << this->kData_sep << "v" << vv << "id";
        }
        for (int vv = 0; vv < this->vll_mgr->sum_num_villages; vv++) {
            *os << this->kData_sep << "v" << vv << "is";
        }
        *os << "\n";
    }

    int infected = 0;
    int infectious = 0;
    int infected_tot = 0;
    int infectious_tot = 0;
    int infected_tot_forest = 0;
    int infectious_tot_forest = 0;
    std::string str_infected;
    std::string str_infectious;
    
    for (int vv = 0; vv < this->vll_mgr->sum_num_villages; vv++) {
        this->vll_mgr->get_mosquito_count_of_village(vv, infected, infectious);
        infected_tot += infected;
        infectious_tot += infectious;
        if (this->vll_mgr->get_location_type(vv) == common::LocationType::kForestPOI
            || this->vll_mgr->get_location_type(vv) == common::LocationType::kForestEntry
            ) {
            infected_tot_forest += infected;
            infectious_tot_forest += infectious;
        }

        str_infected += this->kData_sep;
        str_infected += std::to_string(infected);
        str_infectious += this->kData_sep;
        str_infectious += std::to_string(infectious);
    }

    *os << sim_time;
    *os << this->kData_sep << infected_tot
        << this->kData_sep << infectious_tot
        << this->kData_sep << infected_tot_forest
        << this->kData_sep << infectious_tot_forest
        << this->kData_sep << msq_dp_mgr->get_num_infected()
        << this->kData_sep << msq_dp_mgr->get_num_infectious()
        << str_infected
        << str_infectious
        << "\n";

}

void ReportManager::daily_report_mobility_intensity(
        int sim_time,
        std::ofstream* os
    ) {

    if (sim_time == 0) {
        *os << "day"
            << this->kData_sep << "tot_static"
            << this->kData_sep << "tot_forest"
            << "\n";
    }
    *os << sim_time
        << this->kData_sep << this->vll_mgr->log_daily_num_moves_static
        << this->kData_sep << this->vll_mgr->log_daily_num_moves_forest
        << "\n";

}

void ReportManager::daily_report_mutation(
        const int sim_time,
        std::ofstream* os
    ) {

    if (sim_time == 0) {
        *os << "day" << this->kData_sep;
        *os << "tot_mutation" << this->kData_sep;
        *os << "tot_fixation" << this->kData_sep;
        *os << "tot_import" << this->kData_sep;

        for (const auto& pt: util::Enum_iterator<common::ParasiteType>()) {
            *os << pt << this->kData_sep;
        }
        for (const auto& pt: util::Enum_iterator<common::ParasiteType>()) {
            *os << pt << this->kData_sep;
        }
        *os << "\n";
    }

    std::string str_mutation = "";
    std::string str_fixation = "";

    int tot_mutation = 0;
    int tot_fixation = 0;

    for (const auto& pt: util::Enum_iterator<common::ParasiteType>()) {
        // *os << this->bld_mgr->log_new_mutation[static_cast<int>(pt)] << this->kData_sep;
        int pp_mut = this->bld_mgr->log_new_mutation[static_cast<int>(pt)];
        tot_mutation += pp_mut;
        str_mutation += std::to_string(pp_mut);
        str_mutation += this->kData_sep;
    // }
    // for (const auto& pt: util::Enum_iterator<common::ParasiteType>()) {
        // *os << this->bld_mgr->log_new_fixation[static_cast<int>(pt)] << this->kData_sep;
        int pp_fix = this->bld_mgr->log_new_fixation[static_cast<int>(pt)];
        tot_fixation += pp_fix;
        str_fixation += std::to_string(pp_fix);
        str_fixation += this->kData_sep;
    }

    *os << sim_time << this->kData_sep
        << tot_mutation << this->kData_sep
        << tot_fixation << this->kData_sep
        << this->bld_mgr->report_daily_imported_cases << this->kData_sep
        << str_mutation
        << str_fixation
        << "\n";

}

void ReportManager::daily_report_clearance_natural(
        const int sim_time,
        std::ofstream* os
    ) {

    if (sim_time == 0) {
        *os << "day" << this->kData_sep;

        for (const auto& pt: util::Enum_iterator<common::ParasiteType>()) {
            *os << pt << this->kData_sep;
        }
        *os << "\n";
    }

    *os << sim_time << this->kData_sep;
    for (const auto& pt: util::Enum_iterator<common::ParasiteType>()) {
        *os << this->bld_mgr->log_new_clearance_natural[static_cast<int>(pt)] << this->kData_sep;
    }
    *os << "\n";

}

void ReportManager::daily_report_clearance_drug(
        const int sim_time,
        std::ofstream* os
    ) {

    if (sim_time == 0) {
        *os << "day" << this->kData_sep;

        for (const auto& pt: util::Enum_iterator<common::ParasiteType>()) {
            *os << pt << this->kData_sep;
        }
        *os << "\n";
    }

    *os << sim_time << this->kData_sep;
    for (const auto& pt: util::Enum_iterator<common::ParasiteType>()) {
        *os << this->bld_mgr->log_new_clearance_drug[static_cast<int>(pt)] << this->kData_sep;
    }
    *os << "\n";

}

void ReportManager::daily_report_treatment(
        const int sim_time,
        village::VillageManager* v_mgr,
        human::BloodSystemManager* b_mgr,
        // human::HumanManager* h_mgr,
        std::ofstream* os
    ) {

    if (sim_time == 0) {
        *os << "day" << this->kData_sep;
        *os << "total-clinical" << this->kData_sep;
        *os << "total-asymptomatic" << this->kData_sep;

        *os << "total-treatment" << this->kData_sep;
        *os << "total-new-case-clinical" << this->kData_sep;
        *os << "total-new-case-asymptomatic" << this->kData_sep;

        // for (const auto& pt: util::Enum_iterator<common::ParasiteType>()) {
        //     *os << "new-treatment^{" << pt << "}" << this->kData_sep;
        //     *os << "new-case-clinical^{" << pt << "}" << this->kData_sep;
        //     *os << "new-case-asymptomatic^{" << pt << "}" << this->kData_sep;
        // }

        // *os << "new-case-clinical^{age-5}" << this->kData_sep;
        // *os << "new-case-clinical^{age-10}" << this->kData_sep;
        // *os << "new-case-clinical^{age-15}" << this->kData_sep;
        // *os << "new-case-clinical^{age-15+}" << this->kData_sep;

        *os << "\n";
    }

    *os << sim_time << this->kData_sep;
    *os << b_mgr->sum_clinical_count << this->kData_sep;
    *os << b_mgr->sum_asymptomatic_count << this->kData_sep;

    // std::string str_per_pt="";
    int tot_new_treatment = 0;
    int tot_new_cases_clinical = 0;
    int tot_new_cases_asymptomatic = 0;

    for (const auto& pt: util::Enum_iterator<common::ParasiteType>()) {
        // *os << v_mgr->daily_record_of_new_treatments_by_parasite_type[static_cast<int>(pt)] << this->kData_sep;
        // *os << b_mgr->daily_record_of_new_cases_clinical[static_cast<int>(pt)] << this->kData_sep;
        // *os << b_mgr->daily_record_of_new_cases_asymptomatic[static_cast<int>(pt)] << this->kData_sep;

        int pp = static_cast<int>(pt);

        tot_new_treatment += v_mgr->daily_record_of_new_treatments_by_parasite_type[pp];
        tot_new_cases_clinical += b_mgr->daily_record_of_new_cases_clinical[pp];
        tot_new_cases_asymptomatic += b_mgr->daily_record_of_new_cases_asymptomatic[pp];

    }
    *os << tot_new_treatment << this->kData_sep;
    *os << tot_new_cases_clinical << this->kData_sep;
    *os << tot_new_cases_asymptomatic << this->kData_sep;

    // (void) h_mgr;
    // const int kNum_age_groups = 4;
    // const int* age_of = h_mgr->get_age_of();
    // std::vector<int> cases_per_age_group(kNum_age_groups,0);

    // // std::cout << age_of[0] << "\n";

    // for (size_t ss = 0; ss < b_mgr->report_daily_new_clinical_case.size(); ss++) {
    //     if (b_mgr->report_daily_new_clinical_case[ss]) {
            
    //         if (age_of[ss] <= 5) {
    //             cases_per_age_group[0]++;
    //         } else if (age_of[ss] <= 10) {
    //             cases_per_age_group[1]++;
    //         } else if (age_of[ss] <= 15) {
    //             cases_per_age_group[2]++;
    //         } else {
    //             cases_per_age_group[kNum_age_groups-1]++;
    //         }
            
    //         // b_mgr->report_daily_new_clinical_case[ss] = false;

    //     }
    // }
    // for (const auto& gg: cases_per_age_group) {
    //     *os << gg << this->kData_sep;
    // }

    *os << "\n";

}

void ReportManager::daily_report_mosquito(
        const int sim_time,
        village::VillageManager* v_mgr,
        std::ofstream* os
    ) {

    if (sim_time == 0) {
        *os << "day" << this->kData_sep
            << "+infected_{new}" << this->kData_sep
            << "-infected_{death}" << this->kData_sep
            << "-infectious_{death}" << this->kData_sep
            << "incubation" << this->kData_sep
            << "infected" << this->kData_sep
            << "infectious" << this->kData_sep
            << "+infected_{new-rb}" << this->kData_sep
            << "eggs" << this->kData_sep
            << "larv" << this->kData_sep
            << "imat" << this->kData_sep
            << "adults" << this->kData_sep
            << "adults_biting" << this->kData_sep
            << "cap" << this->kData_sep
            << "clean_biting" << this->kData_sep
            << "new_adults" << this->kData_sep
            << "death_adults" << this->kData_sep
            << "death_adults_infectious_count" << this->kData_sep
            << "pit_death_infected" << this->kData_sep
            << "pit_death_infectious" << this->kData_sep
            << "pit_death_incubation" << this->kData_sep
            << "\n";
    }

    *os << sim_time << this->kData_sep
        <<  v_mgr->daily_record_of_mosquito_newly_infected << this->kData_sep
        <<  v_mgr->daily_record_of_mosquito_death_infected << this->kData_sep
        <<  v_mgr->daily_record_of_mosquito_death_infectious << this->kData_sep
        <<  v_mgr->daily_record_of_mosquito_incubation << this->kData_sep
        <<  v_mgr->daily_record_of_mosquito_infected_sum << this->kData_sep
        <<  v_mgr->daily_record_of_mosquito_infectious_sum << this->kData_sep
        <<  v_mgr->daily_record_of_mosquito_new_rb << this->kData_sep
        <<  v_mgr->daily_record_of_eggs_sum << this->kData_sep
        <<  v_mgr->daily_record_of_larv_sum << this->kData_sep
        <<  v_mgr->daily_record_of_imat_sum << this->kData_sep
        <<  v_mgr->daily_record_of_aliv_sum << this->kData_sep
        <<  v_mgr->daily_record_of_aliv_biting_sum << this->kData_sep
        <<  v_mgr->daily_record_of_max_capacity_sum << this->kData_sep
        <<  v_mgr->daily_record_of_mosquito_clean_sum << this->kData_sep
        <<  v_mgr->daily_record_of_mosquito_new_adults_sum << this->kData_sep
        <<  v_mgr->daily_record_of_mosquito_death_adults_sum << this->kData_sep
        <<  v_mgr->daily_record_of_mosquito_death_adults_infectious_count_sum << this->kData_sep
        <<  msq_dp_mgr->log_step_mosq_pit_death_infected << this->kData_sep
        <<  msq_dp_mgr->log_step_mosq_pit_death_infectious << this->kData_sep
        <<  msq_dp_mgr->log_step_mosq_pit_incubation << this->kData_sep
        << "\n";
}

void ReportManager::daily_report_infection(
        const int sim_time,
        village::VillageManager* v_mgr,
        std::ofstream* os
    ) {

    if (sim_time == 0) {
        *os << "day" << this->kData_sep
            << "bites" << this->kData_sep
            << "infections" << this->kData_sep
            << "\n";
    }

    *os << sim_time << this->kData_sep
        << v_mgr->daily_record_of_new_bites_sum << this->kData_sep
        << v_mgr->daily_record_of_new_infections_sum << this->kData_sep
        << "\n";
    
}

void ReportManager::daily_report_infection_village(
        const int sim_time,
        village::VillageManager* v_mgr,
        std::ofstream* os
    ) {

    if (sim_time == 0) {
        *os << "day" << this->kData_sep;
        for (int vv = 0; vv < v_mgr->sum_num_villages; vv++) {
            *os << "v" << vv << "-bt" << this->kData_sep << "v" << vv << "-if" << this->kData_sep;
        }
        *os << "\n";
    }

    *os << sim_time << this->kData_sep;
    for (int vv = 0; vv < v_mgr->sum_num_villages; vv++) {
        *os << v_mgr->daily_record_of_new_bites.at(vv) << this->kData_sep
            << v_mgr->daily_record_of_new_infections.at(vv) << this->kData_sep;
    }
    *os << "\n";
    
}

void ReportManager::daily_report_drug(
        const int sim_time,
        human::BloodSystemManager* b_mgr,
        village::VillageManager* v_mgr,
        std::ofstream* os
    ) {

    if (sim_time == 0) {
        *os << "day" << this->kData_sep;

        // // Treatment Failure by Drug loss
        // for (const auto& pt: util::Enum_iterator<common::ParasiteType>()) {
        //     *os << "tf_{drug-loss}^{" << pt << "}" << this->kData_sep;
        // }
        // *os << "tf_{drug-loss}" << this->kData_sep;

        // *os << "df_{days}" << this->kData_sep;
        
        *os << "Total_{in}" << this->kData_sep
            << "Total_{current}" << this->kData_sep
            << "Total_{f-rate}" << this->kData_sep
            << "Total_{f-rpt-tmt}" << this->kData_sep
            << "Total_{f-rmn-cln}" << this->kData_sep;

        for (const auto& dd: util::Enum_iterator<common::DrugName>()) {
            *os << dd << "_{in}" << this->kData_sep;
        }
        for (const auto& dd: util::Enum_iterator<common::DrugName>()) {
            *os << dd << "_{current}" << this->kData_sep;
        }
        for (const auto& dd: util::Enum_iterator<common::DrugName>()) {
            *os << dd << "_{f-rpt-tmt}" << this->kData_sep;
        }
        for (const auto& dd: util::Enum_iterator<common::DrugName>()) {
            *os << dd << "_{f-rmn-cln}" << this->kData_sep;
        }

        *os << "\n";
    }

    *os << sim_time << this->kData_sep;

    // int all_rtype_count = 0;
    // int per_rtype_count = 0;
    // for (const auto& pt: util::Enum_iterator<common::ParasiteType>()) {
    //     per_rtype_count = b_mgr->sum_tf_by_drug_loss_count[static_cast<int>(pt)];
    //     *os << per_rtype_count << this->kData_sep;
    //     all_rtype_count += per_rtype_count;
    // }

    // *os << all_rtype_count << this->kData_sep;

    int intake_total = 0;
    int fail_by_new_treatment_total = 0;
    int fail_by_effective_period_total = 0;
    int current_total = 0;

    std::string str_per_drug_intake;
    std::string str_per_drug_current;
    std::string str_per_drug_f1;
    std::string str_per_drug_f2;

    for (const auto& ddn: util::Enum_iterator<common::DrugName>()) {
        int dd = static_cast<int>(ddn);
        // *os << b_mgr->daily_record_of_drug_intake[dd] << this->kData_sep
        //     << v_mgr->daily_record_of_drug_failure_by_new_treatment[dd] << this->kData_sep
        //     << b_mgr->daily_record_of_drug_failure_by_effetive_period[dd] << this->kData_sep
        //     << b_mgr->daily_record_of_drug_current[dd] << this->kData_sep;

        str_per_drug_intake += std::to_string(b_mgr->daily_record_of_drug_intake[dd]);
        str_per_drug_intake += this->kData_sep;
        str_per_drug_current += std::to_string(b_mgr->daily_record_of_drug_current[dd]);
        str_per_drug_current += this->kData_sep;

        str_per_drug_f1 += std::to_string(v_mgr->daily_record_of_drug_failure_by_new_treatment[dd]);
        str_per_drug_f1 += this->kData_sep;
        str_per_drug_f2 += std::to_string(b_mgr->daily_record_of_drug_failure_by_effetive_period[dd]);
        str_per_drug_f2 += this->kData_sep;

        intake_total += b_mgr->daily_record_of_drug_intake[dd];
        fail_by_new_treatment_total += v_mgr->daily_record_of_drug_failure_by_new_treatment[dd];
        fail_by_effective_period_total += b_mgr->daily_record_of_drug_failure_by_effetive_period[dd];
        current_total += b_mgr->daily_record_of_drug_current[dd];

    }

    *os << intake_total << this->kData_sep
        << current_total << this->kData_sep
        << this->daily_report_drug_fail_rate_calc(
                intake_total,
                fail_by_effective_period_total,
                this->kDrug_failure_if_clinical_on_day,
                this->kDrug_failure_measurement_length,
                (sim_time == 0)
            )
        << this->kData_sep
        << fail_by_new_treatment_total << this->kData_sep
        << fail_by_effective_period_total << this->kData_sep;

    *os << str_per_drug_intake
        << str_per_drug_current
        << str_per_drug_f1
        << str_per_drug_f2 << "\n";

    // *os << "\n";
}

float ReportManager::daily_report_drug_fail_rate_calc(
        float today_in,
        float today_fail,

        int drug_failure_if_clinical_on_day,
        size_t measurement_length,
        bool reset
    ) {

    float res_treatment_failure_rate = 0.0;

    static std::queue<int> cache_in;
    static std::queue<int> queue_in;
    static std::queue<int> queue_fail;

    static int sum_in = 0;
    static int sum_fail = 0;

    if (reset) {
        std::queue<int>().swap(cache_in);
        std::queue<int>().swap(queue_in);
        std::queue<int>().swap(queue_fail);
        sum_in = 0;
        sum_fail = 0;
    }

    queue_fail.push(today_fail);
    sum_fail += today_fail;

    if (queue_fail.size() > measurement_length) {
        sum_fail -= queue_fail.front();
        queue_fail.pop();
    }

    cache_in.push(today_in);
    if (cache_in.size() > static_cast<size_t>(drug_failure_if_clinical_on_day-1)) {
        int ii = cache_in.front();
        queue_in.push(ii);
        sum_in += ii;
        cache_in.pop();
    }
    if (queue_in.size() > measurement_length) {
        sum_in -= queue_in.front();
        queue_in.pop();


    }
    if (queue_in.size() == measurement_length){
        if (sum_fail != 0) {
            if (sum_in <= 0) {
                std::cout << "sum_in=" << sum_in << "\n";
                std::cout << "sum_fail=" << sum_fail << "\n";
                throw std::runtime_error("RM: drug fail rate, sum_in invalid\n");
            }
            res_treatment_failure_rate =
                static_cast<float>(sum_fail) / static_cast<float>(sum_in);
        }
    }

    // std::cout << "cache_in.size()=" << static_cast<int>(cache_in.size()) << "\n";
    // std::cout << "queue_in.size()=" << static_cast<int>(queue_in.size()) << "\n";
    // std::cout << "queue_fail.size()=" << static_cast<int>(queue_fail.size()) << "\n";

    return res_treatment_failure_rate;
}


void ReportManager::daily_report_prevalence(
        const int sim_time,
        human::BloodSystemManager* b_mgr,
        std::ofstream* os
    ) {
    // std::ofstream* os = this->filename_stream_map["_summary.csv"];
    if (sim_time == 0) {
        *os << "time_step" << this->kCSV_sep;
        *os << "prevalence" << this->kCSV_sep
            << "prevalence-count(pop-" << b_mgr->sum_num_systems << ")" << this->kCSV_sep;

        *os << "plasmepsin"  << this->kCSV_sep
            << "580Y" << this->kCSV_sep
            << "mdr1" << this->kCSV_sep
            << "184F" << this->kCSV_sep
            << "86Y" << this->kCSV_sep
            << "76T" << this->kCSV_sep;

        for (const auto& pt: util::Enum_iterator<common::ParasiteType>()) {
            if (pt != common::ParasiteType::kNoParasite) {
                *os << "prvl^{" << pt << "}" << this->kCSV_sep;
            }
        }
        for (const auto& pt: util::Enum_iterator<common::ParasiteType>()) {
            *os << "gf^{" << pt << "}" << this->kCSV_sep;
        }
        // *os << "580Y" << this->kCSV_sep
        // *os << "clinical;asymptomatic;";
        *os << "num_pgs" << this->kCSV_sep 
            << "avg_nz_num_of_clinical_pgs_of";
        // *os << "num_pgs_r0;num_pgs_ra;num_pgs_rb;num_pgs_rab;";
        *os << "\n";
    }
    
    if ((sim_time == 2) && (!this->kIf_batch_mode)) {
        os->flush();
        // this->daily_report_prevalence_start_plotting(this->output_prefix + "_prevalence_daily.csv");
    }

    *os << std::to_string(sim_time) << this->kCSV_sep;

    const float sum_num_systems_f = static_cast<float>(b_mgr->sum_num_systems);
    const int all_rtype_count = sum_num_systems_f - b_mgr->sum_prevalence_count[
                                            static_cast<int>(common::ParasiteType::kNoParasite)
                                        ];

    const float prevalence = all_rtype_count/sum_num_systems_f;

    *os << prevalence << this->kCSV_sep
        << all_rtype_count << this->kCSV_sep;
    // int all_rtype_count = 0;
    // int per_rtype_count = 0;
    // *os << b_mgr->daily_record_of_gw_76T << this->kCSV_sep
    //     << b_mgr->daily_record_of_gw_86Y << this->kCSV_sep
    //     << b_mgr->daily_record_of_gw_184F << this->kCSV_sep
    //     << b_mgr->daily_record_of_gw_SndC << this->kCSV_sep
    //     << b_mgr->daily_record_of_gw_580Y << this->kCSV_sep
    //     << b_mgr->daily_record_of_gw_2x << this->kCSV_sep;
    for (const auto& ww : b_mgr->daily_record_of_gw_per_site) {
        *os << ww/static_cast<float>(b_mgr->daily_record_of_parasite_positive_systems)
            << this->kCSV_sep;
    }


    for (const auto& pt: util::Enum_iterator<common::ParasiteType>()) {
        if (pt!=common::ParasiteType::kNoParasite){
            // per_rtype_count = b_mgr->sum_prevalence_count[(int)pt];
            // all_rtype_count += per_rtype_count;
            // *os << (float)per_rtype_count/(float)b_mgr->sum_num_systems << this->kCSV_sep;
            *os << b_mgr->sum_prevalence_count[static_cast<int>(pt)]
                    /sum_num_systems_f << this->kCSV_sep;
        }
    }
    
    // *os << all_rtype_count/(float)b_mgr->sum_num_systems << this->kCSV_sep
    //     << all_rtype_count << this->kCSV_sep;
    for (const auto& pt: util::Enum_iterator<common::ParasiteType>()) {
        *os << b_mgr->daily_record_of_genotype_weights[static_cast<int>(pt)] / (float) b_mgr->daily_record_of_parasite_positive_systems
            << this->kCSV_sep;
    }

    // *os << b_mgr->sum_clinical_count << this->kCSV_sep
    //     << b_mgr->sum_asymptomatic_count << this->kCSV_sep;

    *os << b_mgr->daily_record_of_num_pgs << this->kCSV_sep
        << b_mgr->daily_record_of_avg_clinical_pgs;

    // *os << b_mgr->daily_record_of_num_r0_pgs  << this->kCSV_sep;
    // *os << b_mgr->daily_record_of_num_ra_pgs  << this->kCSV_sep;
    // *os << b_mgr->daily_record_of_num_rb_pgs  << this->kCSV_sep;
    // *os << b_mgr->daily_record_of_num_rab_pgs << this->kCSV_sep;


    *os << "\n";

    std::cout << "prvl: " << prevalence << "\n";
    std::cout << "580Y: " << b_mgr->daily_record_of_gw_per_site[1] << "\n";
    // assert( b_mgr->sum_prevalence_count[(int)common::ParasiteType::kNoParasite]
    //         + all_rtype_count
    //         == b_mgr->sum_num_systems);

    // if (sim_time % this->kOstream_flush_interval == 0 && sim_time!=0) {
    //     os->flush();
    // }

}

void ReportManager::daily_report_prevalence_start_plotting(std::string data_file_name){
    std::string gnu_file_name = this->output_prefix + "_prevalence_daily.gnu";
    std::ofstream os_plot;
    os_plot.open(gnu_file_name);
    os_plot << "set title 'Prevalence Daily (" << this->kOutput_title << ")'\n";
    os_plot << "set datafile separator \"" << this->kData_sep << "\"\n";
    os_plot << "set xrange [0:" << this->sim_time_max << "]\n";
    os_plot << "set yrange [0:" << std::to_string(this->kPrevalence_daily_y_max) << "]\n";
    os_plot << "set format y \"%.0f%%\"\n";
    os_plot << "set ylabel \"Prevalence\"\n";

    // os_plot << "set xlabel \"Time (Day #)\"\n";
    // os_plot << "set mxtics 4\n";
    // os_plot << "show mxtics\n";
    os_plot << "set xtics 365\n";
            
    // Plot at End of Year
    os_plot << "set xtics ('0' 0,"
            << " '1' 365, "
            << " '2' 730, "
            << " '3' 1095, "
            << " '4' 1460, "
            << " '5' 1825, "
            << " '6' 2190, "
            << " '7' 2555, "
            << " '8' 2920, "
            << " '9' 3285, "
            << " '10' 3650, "
            << " '11' 4015, "
            << " '12' 4380, "
            << " '13' 4745, "
            << " '14' 5110, "
            << " '15' 5475, "
            << " '16' 5840, "
            << " '17' 6205, "
            << " '18' 6570, "
            << " '19' 6935, "
            << " '20' 7300, "
            << " '21' 7665, "
            << " '22' 8030, "
            << " '23' 8395, "
            << " '24' 8760, "
            << " '25' 9125"
            << " )\n";
    os_plot << "set xlabel 'Time (End of Year #)'\n";

    // Plot at Middle of Year
    // os_plot << "set xtics ('' 0,"
    //         << " 'Year 1' 183, "
    //         << " 'Year 2' 548, "
    //         << " 'Year 3' 913, "
    //         << " 'Year 4' 1278, "
    //         << " 'Year 5' 1643, "
    //         << " 'Year 6' 2008, "
    //         << " 'Year 7' 2373, "
    //         << " 'Year 8' 2738, "
    //         << " 'Year 9' 2738, "
    //         << " 'Year 10' 2738)\n";
    // os_plot << "set xlabel 'Time'\n";

    os_plot << "set grid\n";
    os_plot << "plot \"" << data_file_name << "\" using 1:(100*$6) with lines title \"All Types\", ";
    os_plot << "\"" << data_file_name << "\" using 1:(100*$3) with lines title \"RA\", ";
    os_plot << "\"" << data_file_name << "\" using 1:(100*$4) with lines title \"RB\", ";
    os_plot << "\"" << data_file_name << "\" using 1:(100*$5) with lines title \"R0\", ";
    // os_plot << "\"" << data_file_name << "\" using 1:(100*$6):(sprintf(\"(%d, %.0f%%)\", $1, 100*$6)) "
    os_plot << "\"" << data_file_name << "\" using 1:(100*$6):(sprintf(\"%.2f%%\", 100*$6)) "
                                        << "every 365::0 with labels point pt 7 offset char 0,1 notitle\n";
    os_plot << "pause " << this->kGnuplot_pause_interval << "\n";
    os_plot << "bind \"s\" \"set terminal pdfcairo enhanced size 5in,4in; set termoption font 'Arial'; set output '"
            << this->output_prefix + "_prevalence_daily.pdf" << "'; replot; unset output; exit gnuplot\"\n";
    os_plot << "reread\n";

    os_plot.close();

    if ( !this->kIf_batch_mode ) {
        if(system(("gnuplot " + gnu_file_name + " &").c_str())){};
    }
}


void ReportManager::daily_report_village(int sim_time){

    std::ofstream* os = this->filename_stream_map["_village_daily.csv"];
    // std::ofstream* os_clr_history = this->filename_stream_map["_parasite_clearance_history.csv"];

    if (sim_time == 0) {
        this->daily_report_village_print_headings(os);
    }

    *os << std::to_string(sim_time);

    // new infections(bites), per village and r-type
    common::ParasiteType* new_infections_list = this->hmn_mgr->get_bitten_by();
    std::map<int, std::map<common::ParasiteType, int>> new_infection_counter;
    std::map<common::ParasiteType, int> new_infection_counter_total;


    for (int hh = 0; hh < this->hmn_mgr->sum_num_humans; hh++) {
        if (new_infections_list[hh] != common::ParasiteType::kNoParasite) {
            new_infection_counter[this->hmn_mgr->get_at_village(hh)][new_infections_list[hh]]++;
            new_infection_counter_total[new_infections_list[hh]]++;
        }
    }

    for(auto pt: util::Enum_iterator<common::ParasiteType>()) {
        *os << ";" + std::to_string(new_infection_counter_total[pt]);
    }
    for (int vv = 0; vv < this->vll_mgr->sum_num_villages; vv++){
        for(auto pt: util::Enum_iterator<common::ParasiteType>()) {
            *os <<  ";" + std::to_string(new_infection_counter[vv][pt]);
        }
    }

    // stage progression, liver to blood, blood to infectious
    common::ParasiteType* pg_type_list = this->bld_mgr->get_pg_type();
    common::StageName* pg_stage_list = this->bld_mgr->get_pg_stage();
    std::map<int, std::map<common::StageName, std::map<common::ParasiteType, int>>> progression_counter;
    std::map<common::StageName, std::map<common::ParasiteType, int>> progression_counter_total;

    for (int pp = 0; pp < this->bld_mgr->sum_num_pgs; pp++){
        if(this->bld_mgr->report_stage_progression_flags[pp]){
            progression_counter[
                this->hmn_mgr->get_at_village(
                    this->bld_mgr->get_system_id_of_pg(pp)
                    )
                ][pg_stage_list[pp]][pg_type_list[pp]]++;

            progression_counter_total[pg_stage_list[pp]][pg_type_list[pp]]++;
        }
    }
    
    for(auto pt: util::Enum_iterator<common::ParasiteType>()) {
        *os << ";" + std::to_string(progression_counter_total[common::StageName::kBlood][pt]);
    }
    for (int vv = 0; vv < this->vll_mgr->sum_num_villages; vv++){
        for(auto pt: util::Enum_iterator<common::ParasiteType>()) {
            *os <<  ";" + std::to_string(progression_counter[vv][common::StageName::kBlood][pt]);
        }
    }
    for(auto pt: util::Enum_iterator<common::ParasiteType>()) {
        *os << ";" + std::to_string(progression_counter_total[common::StageName::kInfectious][pt]);
    }
    for (int vv = 0; vv < this->vll_mgr->sum_num_villages; vv++){
        for(auto pt: util::Enum_iterator<common::ParasiteType>()) {
            *os <<  ";" + std::to_string(progression_counter[vv][common::StageName::kInfectious][pt]);
        }
    }

    // prevalence (according to the dominant type), per village and r-type
    common::ParasiteType* dominant_list = this->bld_mgr->get_dominant_type_of();
    std::map<int, std::map<common::ParasiteType, int>> dominant_type_counter;
    std::map<common::ParasiteType, int> dominant_type_counter_sum;
    for (int hh = 0; hh < this->hmn_mgr->sum_num_humans; hh++) {
        dominant_type_counter[this->hmn_mgr->get_at_village(hh)][dominant_list[hh]]++;
        dominant_type_counter_sum[dominant_list[hh]]++;
    }

    for (auto pt: util::Enum_iterator<common::ParasiteType>()) {
        *os << ";" + std::to_string(dominant_type_counter_sum[pt]);
    }

    for (int vv = 0; vv < this->vll_mgr->sum_num_villages; vv++){
        for(auto pt: util::Enum_iterator<common::ParasiteType>()) {
            *os <<  ";" + std::to_string(dominant_type_counter[vv][pt]);
        }
    }

    *os <<  "\n";
}
void ReportManager::daily_report_village_print_headings(std::ofstream* os){

    *os << "time_step";

    for (auto pt: util::Enum_iterator<common::ParasiteType>()) {
        *os << ";new_bites^{" << pt << "}";
    }

    for (int vv = 0; vv < this->vll_mgr->sum_num_villages; vv++){
        for(auto pt: util::Enum_iterator<common::ParasiteType>()) {
            *os << (";v_{" + std::to_string(vv) + "}^{") << pt << "}" ;
        }
    }

    // l2b
    for (auto pt: util::Enum_iterator<common::ParasiteType>()) {
        *os << ";l2b^{" << pt << "}";
    }
    for (int vv = 0; vv < this->vll_mgr->sum_num_villages; vv++){
        for(auto pt: util::Enum_iterator<common::ParasiteType>()) {
            *os << (";v_{" + std::to_string(vv) + "}^{") << pt << "}" ;
        }
    }

    // b2i
    for (auto pt: util::Enum_iterator<common::ParasiteType>()) {
        *os << ";b2i^{" << pt << "}";
    }
    for (int vv = 0; vv < this->vll_mgr->sum_num_villages; vv++){
        for(auto pt: util::Enum_iterator<common::ParasiteType>()) {
            *os << (";v_{" + std::to_string(vv) + "}^{") << pt << "}" ;
        }
    }

    for (auto pt: util::Enum_iterator<common::ParasiteType>()) {
        *os << ";Prevalence^{" << pt << "}";
    }
    for (int vv = 0; vv < this->vll_mgr->sum_num_villages; vv++) {
        for (auto pt: util::Enum_iterator<common::ParasiteType>()) {
            *os << (";v_{" + std::to_string(vv) + "}^{") << pt << "}" ;
        }
    }

    *os << "\n";
}


void ReportManager::daily_report_on_dominant_type(
        const int sim_time,
        const common::ParasiteType* dominant_type_array,
        const int* at_village_array,
        const int* age_array,
        const int num_humans,
        const int num_villages,
        const int num_age_bins
        ){

    std::ofstream* os_daily_dominant_type_village = this->filename_stream_map["_dominant_type_per_village.csv"];
    std::ofstream* os_daily_dominant_type_age = this->filename_stream_map["_dominant_type_per_age.csv"];
    
    if (sim_time == 0){
        this->daily_report_on_dominant_type_headings(
                num_villages, num_age_bins, os_daily_dominant_type_village, os_daily_dominant_type_age
            );
    }

    // prevalence (according to the dominant type), per village/age and r-type

    std::map<int, std::map<common::ParasiteType, int>> dominant_type_counter_village;
    std::map<int, std::map<common::ParasiteType, int>> dominant_type_counter_age;

    std::map<common::ParasiteType, int> dominant_type_counter_all;

    for (int hh = 0; hh < num_humans; hh++) {
        dominant_type_counter_village[at_village_array[hh]][dominant_type_array[hh]]++;
        dominant_type_counter_age[age_array[hh]][dominant_type_array[hh]]++;

        dominant_type_counter_all[dominant_type_array[hh]]++;
    }

    *os_daily_dominant_type_village << std::to_string(sim_time);
    *os_daily_dominant_type_age << std::to_string(sim_time);

    for (auto pt: util::Enum_iterator<common::ParasiteType>()) {
        *os_daily_dominant_type_village << ";" + std::to_string(dominant_type_counter_all[pt]);
        *os_daily_dominant_type_age << ";" + std::to_string(dominant_type_counter_all[pt]);
    }

    for (int vv = 0; vv < num_villages; vv++) {
        for(auto pt: util::Enum_iterator<common::ParasiteType>()) {
            *os_daily_dominant_type_village <<  ";" + std::to_string(dominant_type_counter_village[vv][pt]);
        }
    }

    for (int aa = 0; aa < num_age_bins; aa++) {
        for(auto pt: util::Enum_iterator<common::ParasiteType>()) {
            *os_daily_dominant_type_age <<  ";" + std::to_string(dominant_type_counter_age[aa][pt]);
        }
    }

    *os_daily_dominant_type_village << "\n";
    *os_daily_dominant_type_age << "\n";

}
void ReportManager::daily_report_on_dominant_type_headings(
        const int num_villages,
        const int num_age_bins,
        std::ofstream* os_village,
        std::ofstream* os_age
        ){

    *os_village << "time_step";
    *os_age << "time_step";

    for (auto pt: util::Enum_iterator<common::ParasiteType>()) {
        *os_village << ";dt^{" << pt << "}";
        *os_age << ";dt^{" << pt << "}";
    }

    for (int vv =0; vv < num_villages; vv++) {
        for (auto pt: util::Enum_iterator<common::ParasiteType>()) {
            *os_village << ";v_{" << vv << "}^{" << pt << "}";
        }
    }

    for (int aa =0; aa < num_age_bins; aa++) {
        for (auto pt: util::Enum_iterator<common::ParasiteType>()) {
            *os_age << ";a_{" << aa << "}^{" << pt << "}";
        }
    }

    *os_village << "\n";
    *os_age << "\n";

}

void ReportManager::daily_report_on_immunity(
        const int sim_time,
        const uint8_t* immunity_level_array,
        const uint8_t* cummulative_exposures_array,
        const int* at_village_array,
        const int num_systems,
        const int num_villages
        ){

    std::ofstream* os_daily_immunity = this->filename_stream_map["_immunity_per_village.csv"];

    if (sim_time == 0){
        this->daily_report_on_immunity_headings(os_daily_immunity, num_villages);
    }

    std::map<int, int> population;
    std::map<int, uint32_t> immunity_level_accumulator;
    std::map<int, uint32_t> cummulative_exposures_accumulator;
    uint32_t immunity_level_all = 0;
    uint32_t cummulative_exposures_all = 0;

    for (int ss = 0; ss < num_systems; ss++) {
        population[at_village_array[ss]]++;
        immunity_level_accumulator[at_village_array[ss]] += immunity_level_array[ss];
        immunity_level_all += immunity_level_array[ss];
        cummulative_exposures_accumulator[at_village_array[ss]] += cummulative_exposures_array[ss];
        cummulative_exposures_all += cummulative_exposures_array[ss];
    }

    *os_daily_immunity << sim_time;
    *os_daily_immunity << ";" << immunity_level_all << "/" << num_systems;
    *os_daily_immunity << ";" << cummulative_exposures_all << "/" << num_systems;


    for (int vv = 0; vv < num_villages; vv++) {
        // *os_daily_immunity << ";" + std::to_string((float)immunity_level_accumulator[vv]/population[vv]);
        *os_daily_immunity << ";" << immunity_level_accumulator[vv] << "/" << population[vv];
        *os_daily_immunity << ";" << cummulative_exposures_accumulator[vv] << "/" << population[vv];
    }

    *os_daily_immunity << "\n";

}
void ReportManager::daily_report_on_immunity_headings(
        std::ofstream* os_immunity,
        const int num_villages
    ){

    *os_immunity << "time_step;immunity_level;cummulative_exposures";

    for (int vv = 0; vv < num_villages; vv++) {
        *os_immunity << ";v_{" << vv << "}";
        *os_immunity << ";v_{" << vv << "}";
    }

    *os_immunity << "\n";

}

void ReportManager::daily_report_on_age_distribution(
        const int sim_time,
        const int* age_array,
        const int num_humans,
        const int num_age_bins
        ){
 
    std::ofstream* os_age_distribution = this->filename_stream_map["_age_distribution.csv"];

    if (sim_time == 0){
        this->daily_report_on_age_distribution_headings(os_age_distribution, num_age_bins);
    }

    std::map<int, int> age_population_counter;
    for (int hh = 0; hh < num_humans; hh++) {
        age_population_counter[age_array[hh]]++;
    }

    *os_age_distribution << sim_time;
    for (int aa = 0; aa < num_age_bins; aa++) {
        *os_age_distribution << ";" << age_population_counter[aa];
    }
    *os_age_distribution << "\n";

}
void ReportManager::daily_report_on_age_distribution_headings(
        std::ofstream* os_age_distribution,
        const int num_age_bins
        ){

    *os_age_distribution << "time_step";

    for (int aa = 0; aa < num_age_bins; aa++) {
        *os_age_distribution << ";" << aa;
    }
    *os_age_distribution << "\n";
}

}