using System.Reflection;
using System.Runtime.CompilerServices;

//
// Le informazioni generali relative a un assembly sono controllate dal seguente 
// insieme di attributi. Per modificare le informazioni associate a un assembly 
// occorre quindi modificare i valori di questi attributi.
//
[assembly: AssemblyTitle("")]
[assembly: AssemblyDescription("")]
[assembly: AssemblyConfiguration("")]
[assembly: AssemblyCompany("")]
[assembly: AssemblyProduct("")]
[assembly: AssemblyCopyright("")]
[assembly: AssemblyTrademark("")]
[assembly: AssemblyCulture("")]		

//
// Le informazioni sulla versione di un assembly sono costituite dai seguenti quattro valori:
//
//      Numero di versione principale
//      Numero di versione secondario 
//      Numero revisione
//      Numero build
//
// È possibile specificare tutti i valori o impostare come predefiniti i valori Numero revisione e Numero build 
// utilizzando l'asterisco (*) come illustrato di seguito:

[assembly: AssemblyVersion("1.0.*")]

//
// Per firmare l'assembly è necessario specificare una chiave da utilizzare.
// Fare riferimento alla documentazione di Microsoft .NET Framework per ulteriori informazioni sulla firma degli assembly.
//
// Utilizzare gli attributi elencati di seguito per verificare la chiave utilizzata per la firma. 
//
// Note: 
//   (*) Se non è specificata alcuna chiave, non sarà possibile firmare l'assembly.
//   (*) KeyName fa riferimento a una chiave installata nel provider di servizi di
//       crittografia (CSP) sul computer in uso. KeyFile fa riferimento a un file che contiene
//       una chiave.
//   (*) Se entrambi i valori KeyFile e KeyName sono specificati, si 
//       verificherà il seguente processo:
//       (1) Se KeyName è presente in CSP, verrà utilizzata tale chiave.
//       (2) Se KeyName non esiste e KeyFile esiste, la chiave 
//           di KeyFile verrà installata nel CSP e utilizzata.
//   (*) Per creare un KeyFile, è possibile utilizzare l'utilità sn.exe (Strong Name).
//       Quando si specifica il KeyFile, il percorso dovrà essere
//       relativo alla directory di output del progetto, ovvero
//       %Project Directory%\obj\<configuration>. Se ad esempio il KeyFile si
//       trova nella directory del progetto, occorre specificare l'attributo AssemblyKeyFile 
//       come [assembly: AssemblyKeyFile("..\\..\\mykey.snk")]
//   (*) La firma ritardata è un'opzione avanzata. Vedere la documentazione di Microsoft
//       .NET Framework per ulteriori informazioni.
//
[assembly: AssemblyDelaySign(false)]
[assembly: AssemblyKeyFile("")]
[assembly: AssemblyKeyName("")]
