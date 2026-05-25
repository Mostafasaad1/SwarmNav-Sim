# AuctionResult.msg
# Declares the winner of a frontier auction

string auction_id               # Auction identifier
string frontier_id              # Frontier that was auctioned
string winner_id                # Winning robot ID
float32 winning_bid             # Winning bid cost (second-price in Vickrey)
