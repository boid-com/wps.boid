# EOSIO WPS

## ACTION

- [`propose`](#action-propose)
- [`vote`](#action-vote)
- [`unvote`](#action-unvote)
- [`cancel`](#action-cancel)

## TABLE

- [`draft`](#table-draft)
- [`proposal`](#table-proposal)

## ACTION `propose`

Submit a WPS proposal

- Authority:  `proposer`

### params

- `{name} proposer` - proposer of proposal
- `{name} proposal_name` - proposal name
- `{string} title` - proposal title
- `{string} proposal_json` - proposal JSON metadata
- `{asset} budget` - monthly budget payment request
- `{uin8_t} payments` - number of monthly payment duration (maximum of 6 months)

### example

```bash
cleos push action eosio.wps propose '["myaccount", "mywps", "My WPS", "{\"category\": \"other\", \"region\": \"global\"}", "500.0000 EOS", 1]' -p myaccount
```

## ACTION `vote`

Vote for a WPS proposal

- Authority:  `voter`

- `{name} voter` - voter
- `{name} proposal_name` - proposal name
- `{name} vote` - vote (yes/no/abstain)

```bash
cleos push action eosio.wps vote '["myaccount", "mywps", "yes"]' -p myaccount
```

## TABLE `proposal`

- `{name} proposer` - proposer of proposal
- `{name} proposal_name` - proposal name
- `{string} title` - proposal title
- `{string} proposal_json` - proposal JSON metadata
- `{asset} budget` - monthly budget payment request
- `{uint8_t} payments` - number of monthly payment duration (maximum of 6 months)
- `{asset} deposit` - deposit required to active proposal
- `{name} status` - current status of proposal (draft/active/completed/expired)
- `{time_point_sec} start` - start of voting period (UTC)
- `{time_point_sec} end` - end of voting period (UTC)

### example

```json
{
  "proposer": "myaccount",
  "proposal_name": "mywps",
  "title": "My WPS",
  "proposal_json": "{\"category\": \"other\", \"region\": \"global\"}",
  "budget": "500.0000 EOS",
  "payments": 1,
  "deposit": "100.0000 EOS",
  "status": "active",
  "start": "2019-11-01T00:00:00",
  "end": "2019-12-01T00:00:00"
}
```

## TABLE `votes`

- `{name} proposal_name` - The proposal's name, its ID among all proposals
- `{vector<name>} yes` - vector array of yes votes
- `{vector<name>} no` - vector array of no votes
- `{vector<name>} abstain` - vector array of abstain votes
- `{int16_t} total_net_votes` - total net votes

### example

```json
{
  "proposal_name": "mywps",
  "yes": ["mybp1", "mybp3", "mybp4"],
  "no": ["mybp2"],
  "abstain": [],
  "total_net_votes": 2
}
```

## TABLE `settings`

- `{time_point_sec} current` - current voting period
- `{uint64_t} vote_margin` - minimum BP vote margin threshold to reach for proposals
- `{asset} deposit_quantity` - deposit required to active proposal
- `{uint64_t} [interval=2592000]` - interval election in seconds

### example

```json
{
  "current": "2019-11-01T00:00:00",
  "vote_margin": 15,
  "deposit_quantity": "100.0000 EOS",
  "interval": 2592000
}
```
