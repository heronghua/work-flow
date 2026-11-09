# Ensure global Git hooks path and hook permissions
_git_hooks_dir="${0:A:h}"
if [ -d "$_git_hooks_dir" ]; then
  _current=$(git config --global --get core.hooksPath 2>/dev/null)
  if [ "$_current" != "$_git_hooks_dir" ]; then
    git config --global core.hooksPath "$_git_hooks_dir" 2>/dev/null || true
  fi
  unset _current

  # Make sure hooks are executable; warn instead of silently chmod.
  for _hook in "$_git_hooks_dir"/*; do
    [ -f "$_hook" ] || continue
    if [ ! -x "$_hook" ]; then
      printf '[git-hooks] not executable: %s\n' "$_hook" >&2
    fi
  done
  unset _hook
fi
unset _git_hooks_dir
