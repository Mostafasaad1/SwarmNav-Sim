# AuctionAnnounce.msg
# Announces a new auction for frontier exploration

string auction_id               # Unique auction identifier
string auctioneer_id            # Robot initiating the auction
Frontier[] frontiers            # Array of frontiers being auctioned
