lay src
tui focus cmd
b MoveIterator_PrivateFields_Allocate
b MoveIterator_PrivateFields_Deallocate
#list ChessAI_ABSearch
#b 116 if i==ROOK0 && src.raw==0 && 3==params->depth
#b 128 if 1==params->depth 
