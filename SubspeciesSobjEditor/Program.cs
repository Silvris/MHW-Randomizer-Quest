using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

namespace SubspeciesSobjEditor
{
    class Program
    {
        static byte[] find = { 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD };
        public static IEnumerable<int> PatternAt(byte[] source, byte[] pattern)
        {
            for (int i = 0; i < source.Length; i++)
            {
                if (source.Skip(i).Take(pattern.Length).SequenceEqual(pattern))
                {
                    yield return i;
                }
            }
        }
        static void EditSobj(FileStream fs)
        {
            byte[] file =  new byte[fs.Length];
            fs.Read(file,0, (int)fs.Length);
            List<int> offsets = PatternAt(file,find).ToList();
            //the first one is *generally* the one we want to edit
            if(offsets.Count > 1)
            {
                fs.Seek(offsets[0]+find.Length, SeekOrigin.Begin);
                //now take input for the id to replace
                Console.Write("Enter the new subspecies ID: ");
                int newId = Convert.ToInt32(Console.ReadLine());
                fs.Write(BitConverter.GetBytes(newId));
            }
            fs.Close();
        }

        static void Main(string[] args)
        {
            if(args.Length > 0)
            {
                //they dropped a file onto the program, or ran cmd
                for(int i = 0; i < args.Length; i++)
                {
                    if (args[i].Contains(".sobj"))
                    {
                        Console.WriteLine("Editing sobj: " + args[i]);
                        EditSobj(new FileStream(args[i], FileMode.Open));
                    }
                    else
                    {
                        Console.WriteLine("File is not an sobj!");
                    }
                }
            }
            else
            {
                Console.Write("Input sobj file path: ");
                string filePath = Console.ReadLine();
                EditSobj(new FileStream(filePath, FileMode.Open));
            }
        }
    }
}
