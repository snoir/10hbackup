# 10hbackup

10hbackup purposes is to make an export of a user Deezer data:
- playlists
- albums (liked)

For each categories, a "main" JSON file containing item list is created (like
`playlists.json`). The detailed JSON of each item is also fetched and written
to it's own file (identified by item's id).
The exported files are added to a git repository, and the changes (if any)
commited at the end of processing. This allows to keep an history of changes
of the exported Deezer data (like additions and deletions on a playlist).

Deezer Rest API is used to achieve that.

## Dependencies

10hbackup requires the following libraries:
- libcurl
- json-c
- libgit2
