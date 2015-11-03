#include "rcppwrap.h"
#include<RcppEigen.h>

template <> flu::vaccine::vaccine_t Rcpp::as( SEXP rVac )
{
    flu::vaccine::vaccine_t vac_cal;
    auto rListVac = Rcpp::as<List>(rVac);
    Rcpp::NumericVector eff = rListVac["efficacy"];
    vac_cal.efficacy_age = Eigen::VectorXd::Zero( eff.size() );
    for (int i = 0; i < eff.size(); ++i)
        vac_cal.efficacy_age[i] = eff[i];

    vac_cal.calendar = Rcpp::as<Eigen::MatrixXd>(rListVac["calendar"]);

    if (rListVac.containsElementNamed("dates")) 
    {
        // Ideally would work with Datetime, but automatic conversion of
        // date vector to datetime vector seems to fail. Even though
        // R does not seem (double check) separate datetime type
        auto rDates = Rcpp::as<Rcpp::DateVector>( rListVac["dates"] )
            .getDates();

        for( auto rDate : rDates )
        {
            vac_cal.dates.push_back(
                    boost::posix_time::ptime(
                        boost::gregorian::date( rDate.getYear(),
                            rDate.getMonth(), rDate.getDay() )
                        // This would be useful if we had an Rcpp::Datetime object
                        //boost::posix_time::time_duration( rDate.getHours(),
                        //    rDate.getMinutes(), rDate.getSeconds() )
                        )
                    );

            /*Rcpp::Rcout << rDate.getYear() << " " <<
                boost::posix_time::to_iso_string(vac_cal.dates.back()) << 
                std::endl;*/
        }
    }
    return vac_cal;
}

template <> flu::contacts::contacts_t Rcpp::as( SEXP rContacts ) 
{
    auto conMatrix = Rcpp::as<Eigen::MatrixXi>( rContacts );

    contacts_t c;

    for(size_t i=0; i<90; i++)
        c.ni[i]=0;

    c.nwe=0;

    /*Loading of the participants with their number of contacts from Polymod*/
    for(int i=0; i<conMatrix.rows(); i++)
    {
        contact_t new_contact;
        new_contact.age = conMatrix(i,0);
        new_contact.weekend = conMatrix(i,1);
        new_contact.N.resize(conMatrix.cols()-2);
        for (size_t j = 0; j < new_contact.N.size(); ++j)
            new_contact.N[j] = conMatrix(i,j+2);
        auto age_part=new_contact.age;
        c.ni[age_part]++;
        if(new_contact.weekend) c.nwe++;

        // TODO remove hard coded age group limits
        new_contact.AG=0;
        if(age_part>0) new_contact.AG++;
        if(age_part>4) new_contact.AG++;
        if(age_part>14) new_contact.AG++;
        if(age_part>24) new_contact.AG++;
        if(age_part>44) new_contact.AG++;
        if(age_part>64) new_contact.AG++;
        new_contact.id = i;

        c.contacts.push_back( new_contact );
    }

    return c;
}

/* 
/// Parameters of the model
struct parameter_set {
    std::vector<double> epsilon = std::vector<double>(5);
    double psi;
    double transmissibility;
    std::vector<double> susceptibility = std::vector<double>(7);
    double init_pop;
};*/
template <> flu::parameter_set Rcpp::as( SEXP rPar ) 
{
    flu::parameter_set pars;
    auto rListPar = Rcpp::as<List>(rPar);
    pars.epsilon = Rcpp::as<Eigen::VectorXd>( rListPar["epsilon"] );
    pars.psi = rListPar["psi"];
    pars.transmissibility = rListPar["transmissibility"];
    pars.susceptibility = Rcpp::as<Eigen::VectorXd>( 
            rListPar["susceptibility"] );
    pars.init_pop = rListPar["init_pop"];
    return pars;
}

template <> SEXP Rcpp::wrap( const flu::parameter_set &parameters )
{
    List rPar;
    rPar["epsilon"] = wrap(parameters.epsilon);
    rPar["psi"] = wrap(parameters.psi);
    rPar["transmissibility"] = wrap(parameters.transmissibility);
    rPar["susceptibility"] = wrap(parameters.susceptibility);
    rPar["init_pop"] = wrap(parameters.init_pop);
    return Rcpp::wrap(rPar);
}

/// Keeps the current state of the model/mcmc
/*struct state_t 
{
    parameter_set parameters;
    double time_infectious, time_latent;

    std::vector<double> positivity_ij = std::vector<double>(260);
    std::vector<size_t> contact_ids;
};*/
template <> flu::state_t Rcpp::as( SEXP rState )
{
    flu::state_t state;
    auto rListState = Rcpp::as<List>(rState);

    state.parameters = Rcpp::as<parameter_set>( rListState["parameters"] );
    state.time_infectious = rListState["time_infectious"];
    state.time_latent = rListState["time_latent"];
    state.contact_ids = Rcpp::as<std::vector<size_t> >( 
            rListState["contact_ids"] );
    if (rListState.containsElementNamed("likelihood"))
        state.likelihood = rListState["likelihood"];

    return state;
}

template <> SEXP Rcpp::wrap( const flu::state_t &sample )
{
    List rState;
    rState["parameters"] = wrap( sample.parameters );
    rState["time_infectious"] = wrap( sample.time_infectious );
    rState["time_latent"] = wrap( sample.time_latent );
    rState["contact_ids"] = wrap( sample.contact_ids );
    rState["likelihood"] = wrap( sample.likelihood );
    return Rcpp::wrap(rState);
}