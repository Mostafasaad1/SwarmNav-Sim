# AuctionBid.msg
# Submits a bid for a frontier in an auction

string auction_id               # Auction identifier
string frontier_id              # Frontier being bid on
string robot_id                 # Robot submitting the bid
float32 bid_cost                # Cost estimate (lower is better in Vickrey)
