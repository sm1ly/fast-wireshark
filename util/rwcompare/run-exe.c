
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "run-exe.h"

/*! \brief  Do nothing. */
static void do_nothing (gpointer user_data) { (void) user_data; }


/*! \brief  Run a program.
 * \param argc  Number of arguments (including executable name).
 * \param argv  Hold executable name and arguments.
 * \param output_ptr  Return value. Will hold program's standard output.
 *                    If NULL, stdout is piped to /dev/null.
 * \return  TRUE iff everything went well.
 */
gboolean run_gather (guint argc, const char* const* argv, char** output_ptr)
{
  char** argv_mutable;
  guint i;
  gboolean successp = TRUE;
  char* outerr = 0;
  char** outerr_ptr = 0;
  GSpawnFlags spawn_flags = G_SPAWN_SEARCH_PATH;

  outerr_ptr = &outerr;

  if (!output_ptr) {
    spawn_flags |= G_SPAWN_STDOUT_TO_DEV_NULL;
  }
  if (!outerr_ptr) {
    spawn_flags |= G_SPAWN_STDERR_TO_DEV_NULL;
  }

  /* Create a mutable copy of argv. */
  argv_mutable = g_malloc ((1+ argc) * sizeof (char*));
  for (i = 0; i < argc; ++i) {
    argv_mutable[i] = g_strdup (argv[i]);
  }
  argv_mutable[argc] = 0;

  successp = g_spawn_sync (0, /* Inherit working directory. */
                           argv_mutable,
                           0, /* Inherit environment. */
                           spawn_flags,
                           &do_nothing,
                           0, /* No data passed to do_nothing() */
                           output_ptr,
                           outerr_ptr,
                           0, /* Don't care about error codes. */
                           0); /* Don't care about the error. */

  /* Free the mutable copy of argv. */
  for (i = 0; i < argc; ++i) {
    g_free (argv_mutable[i]);
  }
  g_free (argv_mutable);

  if (outerr) {
    fprintf (stderr, "%s stderr:\n%s\n",
             argv[0], outerr);
    g_free (outerr);
  }

  return successp;
}


/*! \brief  Run TShark.
 * \param tshark_exe  Name of the TShark executable with optional path.
 * \param pcap_filename  Captured traffic for TShark to read.
 * \param template_filename  Template file used to generate the traffic.
 * \param part  Port number used the plugin.
 * \param output_filename  File to write PDML output.
 * \param duration  Number of seconds to wait for TShark to finish.
 *                  If zero, we are reading from the pcap file.
 *                  Otherwise, we are writing to it.
 * \return  TRUE iff everything went well.
 */
gboolean run_tshark (const char* tshark_exe,
                     const char* pcap_filename,
                     const char* template_filename,
                     int port,
                     const char* output_filename,
                     unsigned duration)
{
  const char* argv[15];
  const char* proto_abbr = "fast";
  char* template_option;
  char* port_option;
  char* duration_option;
  gboolean successp;
  char* output = 0;
  char** output_ptr = 0;

  template_option =
    g_strdup_printf ("%s.template:%s", proto_abbr, template_filename);
  port_option =
    g_strdup_printf ("%s.port:%d",     proto_abbr, port);
  duration_option =
    g_strdup_printf ("duration:%u", duration);

  {
    unsigned argi = 0;
    argv[argi++] = tshark_exe;
    if (template_filename) {
      argv[argi++] = "-o";
      argv[argi++] = template_option;
    }
    if (port) {
      argv[argi++] = "-o";
      argv[argi++] = port_option;
    }
    if (output_filename) {
      argv[argi++] = "-S";
      argv[argi++] = "-T";
      argv[argi++] = "pdml";
    }
    if (duration) {
      argv[argi++] = "-a";
      argv[argi++] = duration_option;
      argv[argi++] = "-i";
      argv[argi++] = "lo";
      argv[argi++] = "-w";
    }
    else {
      argv[argi++] = "-r";
    }
    argv[argi++] = pcap_filename;

    if (output_filename) {
      output_ptr = &output;
    }
    successp = run_gather (argi, argv, output_ptr);
  }

  g_free (template_option);
  g_free (port_option);
  g_free (duration_option);

  /* Write the TShark output to a file. */
  if (successp && output)
  {
    FILE* outf;
    outf = fopen (output_filename, "w+");
    if (outf) {
      fputs (output, outf);
      fclose (outf);
    }
    else {
      successp = FALSE;
    }
  }

  if (output)  g_free (output);

  return successp;
}


struct tshark_args_struct
{
  const char* tshark_exe;
  const char* pcap_filename;
  const char* template_filename;
  int port;
  const char* output_filename;
  unsigned duration;
};
typedef struct tshark_args_struct TSharkArgs;


/*! \brief  Function to run TShark in a new thread.
 *
 * \return  NULL iff something went wrong.
 */
gpointer run_threaded_tshark (gpointer data)
{
  gboolean successp;
  const TSharkArgs* args;
  args = data;
  successp = run_tshark (args->tshark_exe,
                         args->pcap_filename,
                         args->template_filename,
                         args->port,
                         args->output_filename,
                         args->duration);
  g_free (data);

  if (successp)  return data;
  else           return 0;
}

/*! \brief  Spawn a TShark process.
 * \param duration  Number of seconds to wait.
 * \return  A joinable pointer to the spawned thread.
 * \sa  run_threaded_tshark
 */
GThread* spawn_tshark (const char* tshark_exe,
                       const char* pcap_filename,
                       const char* template_filename,
                       int port,
                       const char* output_filename,
                       unsigned duration)
{
  GThread* thread;
  TSharkArgs* args;

  args = g_malloc (sizeof (TSharkArgs));
  args->tshark_exe = tshark_exe;
  args->pcap_filename = pcap_filename;
  args->template_filename = template_filename;
  args->port = port;
  args->output_filename = output_filename;
  args->duration = duration;

  thread = g_thread_create (&run_threaded_tshark,
                            args,
                            TRUE, /* Joinable. */
                            0); /* No errors. */
  return thread;
}


/*! \brief  Run the DataPlanRunner java program.
 */
gboolean run_plan (const char* plan_runner_jar,
                   const char* template_filename,
                   const char* expect_filename,
                   int port)
{
  gboolean successp = TRUE;
  const char* argv[10];
  char* port_option;

  port_option = g_strdup_printf ("%d", port);

  if (successp) {
    unsigned argi = 0;
    argv[argi++] = "java";
    argv[argi++] = "-jar";
    argv[argi++] = plan_runner_jar;
    argv[argi++] = "-t";
    argv[argi++] = template_filename;
    argv[argi++] = "-p";
    argv[argi++] = expect_filename;
    argv[argi++] = "-n";
    argv[argi++] = port_option;
    successp = run_gather (argi, argv, 0);
  }

  g_free (port_option);
  return successp;
}

