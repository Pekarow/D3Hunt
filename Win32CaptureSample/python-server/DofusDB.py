from selenium import webdriver
from selenium.webdriver.common.by import By
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.common.keys import Keys
from selenium.webdriver.support import expected_conditions as EC
from time import sleep
import difflib
def clear(elem):
    elem.send_keys(Keys.CONTROL + "a")
    elem.send_keys(Keys.DELETE)
class DofusDB:
    def __init__(self) -> None:
        # Initialization of selenium connection
        chrome_options = webdriver.ChromeOptions()
        # p = os.path.join(__file__, '..', 'chromedriver.exe')
        # Set the executable path for the Chrome WebDriver
        #chrome_options.add_argument(f"webdriver.chrome.driver=[{p}]")

        self.driver = webdriver.Chrome(options=chrome_options)
       # self.driver.manage().timeouts().implicitlyWait(10, TimeUnit.SECONDS)
       
        self.driver.get("https://dofusdb.fr/fr/tools/treasure-hunt")
        
    def set_x_position(self, pos: int) -> None:
        # Set position of X coordinate in browser
        x_xpath = '/html/body/div[1]/div/div[2]/main/div/div[3]/div[1]/label/div/div[1]/div/input'
        elem = WebDriverWait(self.driver, 30).until(
                 EC.presence_of_element_located((By.XPATH, x_xpath)))
        clear(elem)
        elem.send_keys(pos)
        pass
    def set_y_position(self, pos: int) -> None:
        # Set position of Y coordinate in browser
        y_xpath = '/html/body/div[1]/div/div[2]/main/div/div[3]/div[2]/label/div/div[1]/div/input'
        elem = WebDriverWait(self.driver, 30).until(
                 EC.presence_of_element_located((By.XPATH, y_xpath)))
        clear(elem)
        elem.send_keys(pos)
        pass
    def set_direction(self, pos: int) -> None:
        # Set direction in browser
        top_xpath = '//*[@id="q-app"]/div/div[2]/main/div/div[5]/div[1]/span[1]'
        right_xpath = '//*[@id="q-app"]/div/div[2]/main/div/div[5]/div[1]/span[2]'
        bot_xpath = '//*[@id="q-app"]/div/div[2]/main/div/div[5]/div[2]/span[2]'
        left_xpath = '//*[@id="q-app"]/div/div[2]/main/div/div[5]/div[2]/span[1]'
        
        path  = ""
        if pos == 0:
            path = top_xpath
        elif pos == 1:
            path = right_xpath
        elif pos == 2:
            path = bot_xpath
        elif pos == 3:
            path = left_xpath
            
        elem = WebDriverWait(self.driver, 30).until(
                 EC.presence_of_element_located((By.XPATH, path)))
        elem.click()
        
        pass
    def set_hint(self, hint: str) -> None:
        # Set direction in browser
        hint_xpath = '//*[@id="q-app"]/div/div[2]/main/div/div[7]/div/label/div/div/div[2]/i'
        options_xpath = '/html/body/div[5]'
        hint_input = '/html/body/div[1]/div/div[2]/main/div/div[7]/div/label/div/div/div[1]/div/input'

        elem = WebDriverWait(self.driver, 30).until(
                 EC.presence_of_element_located((By.XPATH, hint_xpath)))
        elem.click()
        elem = WebDriverWait(self.driver, 30).until(
                 EC.presence_of_element_located((By.XPATH, options_xpath)))
        elems = elem.find_elements(By.CLASS_NAME, "q-item__label")
        elems_string = [e.text for e in elems]
        closest = difflib.get_close_matches(word=hint, possibilities=elems_string, n=1, cutoff=.8)
        
        elem = WebDriverWait(self.driver, 30).until(
                 EC.presence_of_element_located((By.XPATH, hint_input)))
        elem.send_keys(closest)
        elem.send_keys(Keys.DOWN)
        elem.send_keys(Keys.ENTER)

    def get_hint_position(self)->"tuple[str,str]":
        
        pos = '//*[@id="q-app"]/div/div[2]/main/div/div[9]/div/div[2]/span'
        elem = WebDriverWait(self.driver, 30).until(
                 EC.presence_of_element_located((By.XPATH, pos)))
        return tuple(elem.text[1:-1].split(','))

if __name__ == "__main__":
    db = DofusDB()
    db.set_x_position(1)
    db.set_y_position(1)
    db.set_direction(1)
    db.set_hint("anneau dor")
    x, y = db.get_hint_position()
    print(x)
    db.set_x_position(int(x))
    db.set_y_position(int(y))
    print()
