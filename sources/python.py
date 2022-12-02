import re
import os
import fnmatch
import pydot 

class ModuleNode(pydot.Node):
    def __init__(self, name, filename = "", obj_dict = None, **attrs):
        self.filename = filename
        pydot.Node.__init__(self, name = name, obj_dict = obj_dict, **attrs)

class CPP_Program(object):
    def __init__(self):
        self.main_name = ""
        self.analyzed_modules = []
        self.nodes = []
        self.graph = None
        self.filenames = []
        
#   It eliminates python comments # 
    def searh(self, line, exp): 

        line = re.split('#', line.strip(),re.ASCII)[0]
        if re.search(exp, line,flags=re.ASCII): 
            return line.strip()
        else:
            return ''

 #  It looks for an expresion in a file and returns a list with occurences
    def search_in_file(self, file, exp):
        try:
            file.seek(0)
        except:
            file = file.split('\n')
            pass
        lines = []

        for line in file:
            if ( not re.search("\s*end\s*module",  line, flags=re.MULTILINE) and 
                 not re.search("\s*end\s*program", line, flags=re.MULTILINE)       ):
              
                l = self.searh(line, exp) 
                if l and ( l not in lines): lines.append(l)
                  
            else:
                break

        return lines

    def search_in_file2(self, file):
        try:
            file.seek(0)
        except:
            file = file.split('\n')
            pass
        lines = []

        for line in file:
           w = line.split(' ')
           if len(w) ==2:
               if w[0] == "import":  
                   module = w[1]
                   module = module.rstrip()
                   if module  not in lines: lines.append(module)

           elif len(w) > 2:
             #if w[0] == "from" and w[2] == "import": 
             #   m = w[1]
             #   ms = m.split('.') 
             #   module = ms[len(ms)-1]
             #   if module  not in lines: lines.append(module)
             if w[0] == "from" and w[2] == "import":  
                 module = w[1]
                 module = module.rstrip()
                 if module  not in lines: lines.append(module)
                #m = w[3] load module name or file name and not function inside module 
               
                #ms = m.split(',') 
                #for module in ms: 
                #  module = module.rstrip()
                #  if module  not in lines: lines.append(module)

                     
             elif w[0] == "import" and w[2] == "as": 
                m = w[1]
                ms = m.split('.') 
                module = ms[len(ms)-1]
                module = module.rstrip()
                if module  not in lines: lines.append(module)
                     


        return lines



      




  # It returns a list with lines  with import 
    def get_uses(self, unitfile, excluded_modules):
        
        #ui = self.search_in_file(unitfile, "^import\s+")
        #uf = 
      
        use_lines = self.search_in_file2(unitfile) #ui + uf 
        #print("lines =", use_lines)
        #exit()
        for i in range(0,len(use_lines)):
            
            if "," in use_lines[i]:
                use_lines[i]=re.split(', only', use_lines[i], flags=re.ASCII)[0]
        uses_list = [re.sub("^import\s+", "", l, flags=re.ASCII) for l in use_lines]
        nmbrexc=0

        #print("uses_list=", uses_list)
        #exit()
        for i in range(0,len(uses_list)):
            i = i - nmbrexc
            if uses_list[i] in excluded_modules:
                del uses_list[i]
                nmbrexc = nmbrexc + 1
            elif uses_list[i] in excluded_modules:
                del uses_list[i]
                nmbrexc = nmbrexc + 1


        #print(" uses list =", uses_list)
        #print(" excluded modules =", excluded_modules)
      
        # first node of caller graph 
        p = unitfile.name.split( os.sep )
        p = p[-1]
        p = p.split(".")
        p = p[0]
            
        # convert from "include datalog.h" to datalog 
        new_list = [ ]
        for u in uses_list: 
            ru = u.replace('"', '')
            ru = ru.replace('.h', '')
            new_list.append(ru)

        if p in new_list: new_list.remove(p) 
       
        for p in excluded_modules: 
            if p in new_list: new_list.remove(p) 

        return new_list

  # It returns the name of the unitfile  
    def get_unit_name(self, unitfile):
        
        name =unitfile.name
        name = name.rsplit( os.sep, 1)
        name = name[-1]
        return name 
  
   
  # It looks for "module" + module_name in file directory 
    def search_module_file(self, module_name, search_directory):
        
        #print("module_name =", module_name)
        for root, dirnames, filenames in os.walk(search_directory):
            for filename in fnmatch.filter(filenames, '*.py'):
                file_name = os.path.join(root, filename)
               
                name = filename.split(".")
                name = name[0]
                if name == module_name: 
                        print( "FOUND module  " + module_name, "  filename =", filename)
                        return file_name
                else:
                        continue



  # If generates a dictionary with USE dependencies. 
  # KEY is the name of the used module  
  # VALUE is a tuple = (name of the file where it is located, dicctionary of its dependencies )    
    def create_uses_dictionary(self, unit_name, unit_filename, search_directory, excluded_modules): 
       
        D = {}
        with open(unit_filename) as unit_file:
            uses = self.get_uses(unit_file, excluded_modules)

        self.analyzed_modules.append(unit_name)

        #print("Analyzed modules =", self.analyzed_modules) 
        #exit() 

        print(" modules =", uses) 
        for module in uses:
            use_filename = self.search_module_file(module, search_directory)
            D[module] = []
            #print("use filename =", use_filename) 
            #input("Press Enter to continue...")

            if use_filename is not None:
                if module not in self.analyzed_modules:
                    self.filenames.append(use_filename)
                    Dn = self.create_uses_dictionary( module, use_filename, search_directory, excluded_modules )
                    D[module] = (use_filename, Dn)
                                              
            else:
                D[module] = ()
        return D



    def create_tree(self, node1, D1):

        for i in D1:
            node2 = ModuleNode(name = i)
            edge = pydot.Edge(node1, node2)

            self.nodes.append(node2)
            self.graph.add_node(node2)
            self.graph.add_edge(edge)

            if D1[i]:
                filename = D1[i][0]
                D2 = D1[i][1]
                node2.filename = filename
                node2.set('shape' , "Mrecord")
                self.graph.get_node(i)[0].set('shape' , "Mrecord")
                self.create_tree(node2, D2)
            else:
                pass



def generate_diagrams(mainfile, dirname, excluded_modules):

    D = CPP_Program( )
    D.filenames.append( mainfile )
    with open(mainfile) as main_file:
       D.main_name = D.get_unit_name( main_file )

  
    D.uses_dictionary = D.create_uses_dictionary( D.main_name, mainfile, dirname, excluded_modules )
    #print(" CPP caller graph Dictionary ") 
    #print("   main_name =", D.main_name) 
    #print("   analyzed_modules =", D.analyzed_modules) 
    #print("   filenames =", D.filenames) 

    D.graph = pydot.Dot(grap_name = "TOP-DOWN design", graph_type = "digraph", dpi = 300 )
    main_node = ModuleNode(name = D.main_name, filename = mainfile, shape = "Mrecord")

    D.nodes.append(main_node)
    D.graph.add_node(main_node)

    D.create_tree(main_node, D.uses_dictionary)
        
    D.G = pydot.Graph(margin=5.)
    D.graph.write_pdf('doc' + os.sep + 'graphs' + os.sep + 'uses_simplediagram.pdf' )
    D.graph.write_dot('doc' + os.sep + 'graphs' +os.sep + 'uses_simplediagram.dot')
    print("**** End simple uses")
