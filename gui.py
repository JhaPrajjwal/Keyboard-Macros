from tkinter import *
import subprocess

class Window(Frame):
    def __init__(self, master=None):
        Frame.__init__(self, master)
        self.master = master
        self.init_window()

    def init_window(self):
        self.master.title("Macro Manager")
        self.pack(fill=BOTH, expand=1)
        add_macro_button = Button(self, text="Add New Macro", command=lambda: self.add_macro_window())
        add_macro_button.place(x=0,y=0)
    
    def add_macro_window(self):
        newWindow = Toplevel(self.master)
        label = Label(newWindow, text='Add New Macro')
        label.pack()

        startButton = Button(newWindow, text="Start", command=lambda: self.write_to_proc_file('start'))
        startButton.place(x=50,y=130)

        endButton = Button(newWindow, text="End", command=lambda: self.write_to_proc_file('end'))
        endButton.place(x=230,y=130)

        startMacroButton = Button(newWindow, text="Start Macro", command=lambda: self.write_to_proc_file('startFullMacro'))
        startMacroButton.place(x=50,y=200)

        endMacroButton = Button(newWindow, text="End Macro", command=lambda: self.write_to_proc_file('endFullMacro'))
        endMacroButton.place(x=230,y=200)

        backButton = Button(newWindow, text="Back", command=newWindow.destroy)
        backButton.place(x=110, y=270)
        newWindow.geometry("300x300")
        


    def write_to_proc_file(self, cmd):
        subprocess.check_call(["./write_proc", cmd])

if __name__ == '__main__':
    root = Tk()
    root.geometry("400x400")
    app = Window(root)
    root.mainloop()