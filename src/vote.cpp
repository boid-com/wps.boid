[[eosio::action]]
void wps::vote( const name voter, const name proposal_name, const name vote )
{
    require_auth( voter );

    // validate proposal if eligible to vote on
    check_proposal_can_vote( proposal_name );

    // cannot vote during completed voting period phase
    check_voting_period_completed();

    // voter must be an active producer with over 100 EOS in total votes
    check_voter_eligible( voter );

    // update `votes` table & eligible voters
    // any existing votes with voters with less than 100 EOS vpay will be excluded
    update_vote( voter, proposal_name, vote, get_eligible_producers() );

    // update `proposals::eligible` field for all active proposals
    update_eligible_proposals();
}

void wps::check_voter_eligible( const name voter )
{
    eosiosystem::producers_table _producers( "eosio"_n, "eosio"_n.value );

    auto prod = _producers.get( voter.value, "[voter] must be registered as a producer" );

    check( prod.is_active, "[voter] must be an active producer");
    check( prod.total_votes > 0.0, "[voter] must have votes");

    const int64_t producer_per_vote_pay = calculate_producer_per_vote_pay( prod.total_votes );
    check( producer_per_vote_pay >= 1000000, "[voter] must be have a vpay of 100 EOS or above");
}

void wps::check_proposal_can_vote( const name proposal_name )
{
    auto proposals_itr = _proposals.find( proposal_name.value );
    check( proposals_itr != _proposals.end(), "[proposal_name] does not exist");
    check( proposals_itr->start_voting_period <= current_time_point(), "[proposal_name] has not yet started");
    check( proposals_itr->status == "active"_n, "[proposal_name] must be active");
}

void wps::update_vote( const name voter, const name proposal_name, const name vote, const set<name> eligible_producers )
{
    // validate vote
    auto votes_itr = _votes.find( proposal_name.value );

    check( votes_itr != _votes.end(), "[proposal_name] votes does not exist");
    check( vote == "yes"_n || vote == "no"_n || vote == "abstain"_n, "[vote] invalid (ex: yes/no/abstain)");

    // update vote
    _votes.modify( votes_itr, same_payer, [&]( auto& row ) {
        check(row.votes[voter] != vote, "[vote] has not been modified");
        row.votes[voter] = vote;
        update_total_net_votes( proposal_name, row.votes, eligible_producers );
    });
}

void wps::update_eligible_proposals()
{
    // settings
    auto settings = _settings.get();
    auto state = _state.get();

    // containers
    eosio::asset total_payout = asset{ 0, symbol{ "EOS", 4 }};

    // filter out min voting threshold proposals
    std::map<int16_t, std::set<eosio::name>> proposals = sort_proposals_by_net_votes( "active"_n );

    // iterate proposals from highest to lowest net votes
    for ( auto itr = proposals.rbegin(); itr != proposals.rend(); ++itr ) {

        // iterate over proposals
        for ( auto proposal_name : itr->second ) {
            // proposal variables
            auto proposal_itr = _proposals.find( proposal_name.value );
            const int16_t total_net_votes = itr->first;
            const eosio::asset monthly_budget = proposal_itr->monthly_budget;

            // min requirements for payouts
            const bool is_min_vote_margin = total_net_votes >= settings.vote_margin;
            const bool is_enough_budget = (total_payout + monthly_budget) <= settings.max_monthly_budget;

            // set eligible of proposal (true/false)
            _proposals.modify( proposal_itr, same_payer, [&]( auto& row ) {
                if ( is_enough_budget && is_min_vote_margin ) {
                    total_payout += monthly_budget;
                    row.eligible = true;
                } else {
                    row.eligible = false;
                }
            });
        }
    }
}

std::map<int16_t, std::set<eosio::name>> wps::sort_proposals_by_net_votes( const eosio::name status )
{
    auto index = _proposals.get_index<"bystatus"_n>();

    // add proposals to ordered map
    std::map<int16_t, std::set<eosio::name>> proposals;

    for ( auto itr = index.lower_bound(status.value); itr != index.upper_bound(status.value); ++itr ) {
        proposals[itr->total_net_votes].insert(itr->proposal_name);
    }
    return proposals;
}


int16_t wps::calculate_total_net_votes( const std::map<name, name> votes, const set<name> eligible_producers )
{
    int16_t total_net_votes = 0;
    for (std::pair<name, name> item : votes) {
        const name voter = item.first;
        const name vote = item.second;

        // illegible voters are skipped in calculation (value=0)
        if ( eligible_producers.find( voter ) == eligible_producers.end() ) continue;
        if (vote == "yes"_n) total_net_votes += 1;
        else if (vote == "no"_n) total_net_votes -= 1;
    }
    return total_net_votes;
}

bool wps::update_total_net_votes( const name proposal_name, const std::map<name, name> votes, const set<name> eligible_producers )
{
    auto proposal_itr = _proposals.find( proposal_name.value );
    check( proposal_itr != _proposals.end(), "[update_total_net_votes::proposal_name] is not found");

    // re-caculate votes
    const int16_t total_net_votes = calculate_total_net_votes( votes, eligible_producers );
    if ( proposal_itr->total_net_votes != total_net_votes ) {
        _proposals.modify( proposal_itr, same_payer, [&]( auto& row ) {
            row.total_net_votes = total_net_votes;
        });
        // modified
        return true;
    }
    // not modified
    return false;
}

int64_t wps::calculate_producer_per_vote_pay( const double total_votes )
{
    eosiosystem::global_state_singleton _gstate( "eosio"_n, "eosio"_n.value );
    auto gstate = _gstate.get();

    return int64_t(( gstate.pervote_bucket * total_votes ) / gstate.total_producer_vote_weight );
}