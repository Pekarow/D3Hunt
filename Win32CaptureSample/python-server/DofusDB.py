from selenium import webdriver
from selenium.webdriver.common.by import By
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.common.keys import Keys
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.common.action_chains import ActionChains
import undetected_chromedriver as uc
import re
import unicodedata
def strip_accents(s):
   return ''.join(c for c in unicodedata.normalize('NFD', s)
                  if unicodedata.category(c) != 'Mn')

from time import sleep
import difflib
x_xpath = '/html/body/div[1]/div/div/main/div/div[3]/div[1]/label/div/div[1]/div/input'
y_xpath = '/html/body/div[1]/div/div/main/div/div[3]/div[2]/label/div/div[1]/div/input'
pattern = r'-?[0-9]{1,2},-?[0-9]{1,2}'
regex = re.compile(pattern)
def clear(elem):
    elem.send_keys(Keys.CONTROL + "a")
    elem.send_keys(Keys.DELETE)
class DofusDB:
    def __init__(self, headless=True) -> None:
        # Initialization of selenium connection
        chrome_options = uc.ChromeOptions()
        chrome_options.add_argument('--disable-extensions')
        chrome_options.add_argument('--profile-directory=Default')
        chrome_options.add_argument("--incognito")
        chrome_options.add_argument("--disable-plugins-discovery")
        chrome_options.add_argument("--start-maximized")
        if headless:
            chrome_options.add_argument("--headless=new")
        self.driver = uc.Chrome(options=chrome_options, use_subprocess=False)
        self.driver.delete_all_cookies()
        self.driver.get("https://dofusdb.fr/fr/tools/treasure-hunt")
    def set_x_position(self, pos: int) -> None:
        # Set position of X coordinate in browser
        elem = WebDriverWait(self.driver, 30).until(
                 EC.presence_of_element_located((By.XPATH, x_xpath)))
        clear(elem)
        elem.send_keys(pos)

    def set_y_position(self, pos: int) -> None:
        # Set position of Y coordinate in browser
        elem = WebDriverWait(self.driver, 30).until(
                 EC.presence_of_element_located((By.XPATH, y_xpath)))
        clear(elem)
        elem.send_keys(pos)

    def set_direction(self, pos: int) -> None:
              
        actions = ActionChains(self.driver)
        if pos == 0:
            actions.send_keys(Keys.ARROW_UP)
        elif pos == 1:
            actions.send_keys(Keys.ARROW_RIGHT)
        elif pos == 2:
            actions.send_keys(Keys.ARROW_DOWN)
        elif pos == 3:
            actions.send_keys(Keys.ARROW_LEFT)
        actions.perform()
        sleep(1)

    """Set hint based on available choices"""
    def set_hint(self, hint: str) -> None:

        hint = hint.replace("\n\n", " ")
        hint = hint.strip()

        # Wait that dropdown appears with all possible values
        _ = WebDriverWait(self.driver, 30).until(
                 EC.presence_of_element_located((By.CLASS_NAME, "q-item__label")))
        elems = self.driver.find_elements(By.CLASS_NAME, "q-item__label")
        # Generate list without accents of the dropdown to match with given hint
        elems_string = [strip_accents(e.text) for e in elems]
        # Fetch hint that ressembles the most to the hint
        closest = difflib.get_close_matches(word=hint, possibilities=elems_string, n=1, cutoff=.4)
        closest_index = elems_string.index(closest[0])
        # click on this element to validate the choice
        elems[closest_index].click()
        
    def get_hint_position(self)->"tuple[str,str]":
        
        elem = WebDriverWait(self.driver, 30).until(
                EC.presence_of_element_located((By.TAG_NAME, "main")))
        results = regex.findall(elem.text, re.DOTALL)
        # print("----------------------")
        # print(elem.text)
        # print("----------------------")
        print(results)
        return tuple(results[-1].split(','))
    
    def run(self, x, y, direction, hint, **_):
        self.set_x_position(x)
        self.set_y_position(y)
        body = self.driver.find_element(By.TAG_NAME, "body")
        body.send_keys(Keys.TAB)
        self.set_direction(direction)
        actions = ActionChains(self.driver)
        actions.send_keys(Keys.DOWN)
        actions.perform()
        self.set_hint(hint)
    # def run(self, data):
    #     run(data['x'], )
        
if __name__ == "__main__":
    db = DofusDB(False)
    a = {'x': -12, 'y': -23, 'direction': 2, 'exit': 0, 'hint': 'Tr  re  rayures '}
    a = {'x': -58, 'y': -52, 'direction': 3, 'exit': 0, 'hint': 'Tissu  carreaux noue '}
    db.run(**a)
    sleep(1)
    x, y = db.get_hint_position()
    print(x, y)
    # db.set_x_position(int(x))
    # db.set_y_position(int(y))
    
