# 10hbackup

10hbackup purposes is to make an export of a user Deezer data:
- playlists
- albums (liked)
- artists (liked)

For each categories, a "main" JSON containing item list is created on disk
(like `playlists.json`). The detailed JSON of each item is also fetched and
written to it's own file (identified by item's id).

Deezer Rest API is used to achieve that.

## Dependencies

10hbackup requires the following libraries:
- libcurl
- json-c

## Todo

- Read the Deezer token from a file
- Store data in a Git repository (using libgit2)
