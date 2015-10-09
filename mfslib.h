int mx_mount (struct m_state * state);
int mx_lookup_path (struct m_state * state, const char * path);
int mx_readdir (off_t offset, const char * path);
int mx_read (char * buf, size_t size, off_t offset, struct entry * entryp);
